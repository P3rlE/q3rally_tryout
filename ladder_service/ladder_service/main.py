"""FastAPI application exposing the Ladder webservice."""
from __future__ import annotations

import base64
import hashlib
import hmac
import json
import os
import secrets
import time
from contextlib import asynccontextmanager
from datetime import datetime, timedelta, timezone
from typing import AsyncIterator

from fastapi import Depends, FastAPI, HTTPException, Path, Query, Request, status
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from sqlalchemy import Engine, delete, select
from sqlalchemy.exc import IntegrityError
from sqlalchemy.orm import Session, sessionmaker

from .db import create_session_factory, session_scope
from .models import Base, Match, RefreshToken, User
from .schemas import (
    AuthTokens,
    ErrorEnvelope,
    LoginRequest,
    MatchCreate,
    ProfileEnvelope,
    ProfileV1,
    RefreshRequest,
    RegisterRequest,
)


ACCESS_TOKEN_TTL_SECONDS = int(os.getenv("LADDER_ACCESS_TOKEN_TTL", "900"))
REFRESH_TOKEN_TTL_SECONDS = int(os.getenv("LADDER_REFRESH_TOKEN_TTL", "604800"))
AUTH_LIMIT_WINDOW_SECONDS = int(os.getenv("LADDER_AUTH_RATE_WINDOW", "60"))
AUTH_LIMIT_MAX_REQUESTS = int(os.getenv("LADDER_AUTH_RATE_MAX", "20"))


def _init_session_factory() -> tuple[sessionmaker[Session], Engine]:
    db_path = os.getenv("LADDER_DB_PATH")
    engine, factory = create_session_factory(db_path)
    Base.metadata.create_all(bind=engine)
    return factory, engine


def _get_server_keys() -> set[str]:
    raw = os.getenv("LADDER_SERVER_API_KEYS", "dev-server-key")
    return {key.strip() for key in raw.split(",") if key.strip()}


def _token_secret() -> str:
    return os.getenv("LADDER_TOKEN_SECRET", "q3rally-dev-secret")


def _password_secret() -> str:
    return os.getenv("LADDER_PASSWORD_PEPPER", "q3rally-dev-pepper")


SessionFactory, Engine = _init_session_factory()
_AUTH_RATE_STATE: dict[str, list[float]] = {}


def get_session() -> Session:
    """Provide a scoped SQLAlchemy session for FastAPI dependencies."""

    with session_scope(SessionFactory) as session:
        yield session


@asynccontextmanager
async def lifespan(_: FastAPI) -> AsyncIterator[None]:
    Base.metadata.create_all(bind=Engine)
    yield


app = FastAPI(
    title="Q3Rally Ladder Service",
    version="0.2.0",
    lifespan=lifespan,
)


def _error(status_code: int, code: str, message: str) -> HTTPException:
    return HTTPException(status_code=status_code, detail={"error": {"code": code, "message": message}})


@app.exception_handler(HTTPException)
async def http_exception_handler(_: Request, exc: HTTPException) -> JSONResponse:
    detail = exc.detail
    if isinstance(detail, dict) and "error" in detail:
        body = detail
    else:
        body = {"error": {"code": "http_error", "message": str(detail)}}
    return JSONResponse(status_code=exc.status_code, content=body)


@app.exception_handler(RequestValidationError)
async def validation_exception_handler(_: Request, exc: RequestValidationError) -> JSONResponse:
    first_error = exc.errors()[0] if exc.errors() else {"msg": "Invalid request"}
    message = str(first_error.get("msg", "Invalid request"))
    return JSONResponse(
        status_code=status.HTTP_400_BAD_REQUEST,
        content={"error": {"code": "invalid_request", "message": message}},
    )


def _extract_bearer_token(request: Request) -> str:
    auth_header = request.headers.get("Authorization", "")
    parts = auth_header.split(" ", 1)
    if len(parts) != 2 or parts[0].lower() != "bearer" or not parts[1].strip():
        raise _error(status.HTTP_401_UNAUTHORIZED, "unauthorized", "Missing or invalid bearer token")
    return parts[1].strip()


def require_server_key(request: Request) -> str:
    token = _extract_bearer_token(request)
    if token not in _get_server_keys():
        raise _error(status.HTTP_401_UNAUTHORIZED, "unauthorized", "Invalid server token")
    return token


def _now_utc() -> datetime:
    return datetime.now(timezone.utc)


def _hash_password(password: str) -> str:
    salt = secrets.token_hex(16)
    derived = hashlib.pbkdf2_hmac(
        "sha256", f"{password}{_password_secret()}".encode("utf-8"), salt.encode("utf-8"), 120_000
    )
    return f"pbkdf2_sha256${salt}${base64.urlsafe_b64encode(derived).decode('utf-8')}"


def _verify_password(password: str, password_hash: str) -> bool:
    try:
        _, salt, value = password_hash.split("$", 2)
    except ValueError:
        return False
    derived = hashlib.pbkdf2_hmac(
        "sha256", f"{password}{_password_secret()}".encode("utf-8"), salt.encode("utf-8"), 120_000
    )
    candidate = base64.urlsafe_b64encode(derived).decode("utf-8")
    return secrets.compare_digest(candidate, value)


def _encode_access_token(user_id: str) -> str:
    exp = int(time.time()) + ACCESS_TOKEN_TTL_SECONDS
    payload = f"{user_id}:{exp}"
    signature = hmac.new(_token_secret().encode("utf-8"), payload.encode("utf-8"), hashlib.sha256).hexdigest()
    token_raw = f"usr.{payload}:{signature}"
    return base64.urlsafe_b64encode(token_raw.encode("utf-8")).decode("utf-8")


def _decode_access_token(token: str) -> str:
    try:
        decoded = base64.urlsafe_b64decode(token.encode("utf-8")).decode("utf-8")
        if not decoded.startswith("usr."):
            raise ValueError("bad prefix")
        payload_sig = decoded[4:]
        payload, signature = payload_sig.rsplit(":", 1)
        expected_sig = hmac.new(
            _token_secret().encode("utf-8"), payload.encode("utf-8"), hashlib.sha256
        ).hexdigest()
        if not secrets.compare_digest(signature, expected_sig):
            raise ValueError("bad signature")
        user_id, exp_text = payload.split(":", 1)
        if int(exp_text) < int(time.time()):
            raise ValueError("expired")
        return user_id
    except Exception as exc:  # noqa: BLE001
        raise _error(status.HTTP_401_UNAUTHORIZED, "unauthorized", "Invalid or expired user token") from exc


def _create_refresh_token(session: Session, user: User) -> str:
    plain = secrets.token_urlsafe(48)
    token_hash = hashlib.sha256(plain.encode("utf-8")).hexdigest()
    expires_at = _now_utc() + timedelta(seconds=REFRESH_TOKEN_TTL_SECONDS)
    db_token = RefreshToken(user_id=user.id, token_hash=token_hash, expires_at=expires_at, revoked=False)
    session.add(db_token)
    session.flush()
    return plain


def _check_auth_rate_limit(request: Request) -> None:
    key = f"{request.client.host if request.client else 'unknown'}:{request.url.path}"
    now = time.time()
    bucket = _AUTH_RATE_STATE.get(key, [])
    recent = [stamp for stamp in bucket if now - stamp <= AUTH_LIMIT_WINDOW_SECONDS]
    if len(recent) >= AUTH_LIMIT_MAX_REQUESTS:
        raise _error(status.HTTP_429_TOO_MANY_REQUESTS, "rate_limited", "Too many auth requests")
    recent.append(now)
    _AUTH_RATE_STATE[key] = recent


def require_user_token(request: Request, session: Session = Depends(get_session)) -> User:
    token = _extract_bearer_token(request)
    user_id = _decode_access_token(token)
    user = session.execute(select(User).where(User.user_id == user_id)).scalar_one_or_none()
    if user is None:
        raise _error(status.HTTP_401_UNAUTHORIZED, "unauthorized", "Unknown user")
    return user


def _normalize_profile(raw_profile: str | None, user_id: str) -> ProfileV1:
    defaults = {
        "schema_version": "v1",
        "name": user_id,
        "country": None,
        "birthdate": None,
        "gender": "unspecified",
        "avatar_ref": None,
    }
    if not raw_profile:
        return ProfileV1(**defaults)

    payload = json.loads(raw_profile)
    if "schema_version" not in payload and "name" in payload:
        payload = {
            "schema_version": "v1",
            "name": payload["name"],
            "country": None,
            "birthdate": None,
            "gender": "unspecified",
            "avatar_ref": payload.get("avatar_ref"),
        }
    return ProfileV1(**{**defaults, **payload})


@app.post(
    "/api/v1/matches",
    status_code=status.HTTP_201_CREATED,
    response_class=JSONResponse,
)
def create_match(
    match: MatchCreate,
    _: str = Depends(require_server_key),
    session: Session = Depends(get_session),
) -> dict[str, str]:
    """Store a reported match in the database."""

    db_match = Match(
        match_id=match.matchId,
        mode=match.mode,
        start_time=match.startTime,
        end_time=match.endTime,
        payload=match.json(),
    )
    session.add(db_match)
    try:
        session.flush()
    except IntegrityError as exc:
        session.rollback()
        raise _error(status.HTTP_409_CONFLICT, "match_exists", "matchId already exists") from exc

    return {"matchId": match.matchId}


@app.get("/api/v1/matches")
def list_matches(
    _: str = Depends(require_server_key),
    session: Session = Depends(get_session),
    mode: str | None = Query(default=None, description="Filter by gametype"),
    limit: int = Query(default=50, ge=1, le=200),
    offset: int = Query(default=0, ge=0),
) -> dict[str, list[dict[str, object]]]:
    """Return a paginated list of matches."""

    query = select(Match).order_by(Match.start_time.desc()).offset(offset).limit(limit)
    if mode:
        query = query.where(Match.mode == mode)

    rows = session.execute(query).scalars().all()
    matches = [json.loads(row.payload) for row in rows]
    return {"matches": matches}


@app.get("/api/v1/matches/{match_id}")
def get_match(
    match_id: str = Path(..., description="The matchId that was originally submitted"),
    _: str = Depends(require_server_key),
    session: Session = Depends(get_session),
) -> dict[str, object]:
    """Return a single match payload."""

    query = select(Match).where(Match.match_id == match_id)
    result = session.execute(query).scalar_one_or_none()
    if result is None:
        raise _error(status.HTTP_404_NOT_FOUND, "not_found", "Match not found")

    payload = json.loads(result.payload)
    payload["createdAt"] = result.created_at.isoformat()
    return payload


@app.delete(
    "/api/v1/matches/{match_id}",
    status_code=status.HTTP_204_NO_CONTENT,
)
def delete_match(
    match_id: str = Path(..., description="The matchId to delete"),
    _: str = Depends(require_server_key),
    session: Session = Depends(get_session),
) -> JSONResponse:
    """Delete a match from the database."""

    query = select(Match).where(Match.match_id == match_id)
    result = session.execute(query).scalar_one_or_none()
    if result is None:
        raise _error(status.HTTP_404_NOT_FOUND, "not_found", "Match not found")

    session.delete(result)
    session.flush()
    return JSONResponse(status_code=status.HTTP_204_NO_CONTENT, content={})


@app.post("/api/v1/auth/register", response_model=AuthTokens, responses={429: {"model": ErrorEnvelope}})
def register_user(
    payload: RegisterRequest,
    request: Request,
    session: Session = Depends(get_session),
) -> AuthTokens:
    _check_auth_rate_limit(request)
    existing = session.execute(select(User).where(User.user_id == payload.user_id)).scalar_one_or_none()
    if existing is not None:
        raise _error(status.HTTP_409_CONFLICT, "user_exists", "user_id already registered")

    profile = ProfileV1(name=payload.user_id, gender="unspecified")
    user = User(user_id=payload.user_id, password_hash=_hash_password(payload.password), profile=profile.json())
    session.add(user)
    session.flush()

    access_token = _encode_access_token(user.user_id)
    refresh_token = _create_refresh_token(session, user)
    return AuthTokens(
        access_token=access_token,
        refresh_token=refresh_token,
        expires_in=ACCESS_TOKEN_TTL_SECONDS,
    )


@app.post("/api/v1/auth/login", response_model=AuthTokens, responses={429: {"model": ErrorEnvelope}})
def login_user(
    payload: LoginRequest,
    request: Request,
    session: Session = Depends(get_session),
) -> AuthTokens:
    _check_auth_rate_limit(request)
    user = session.execute(select(User).where(User.user_id == payload.user_id)).scalar_one_or_none()
    if user is None or not _verify_password(payload.password, user.password_hash):
        raise _error(status.HTTP_401_UNAUTHORIZED, "invalid_credentials", "Invalid user_id or password")

    access_token = _encode_access_token(user.user_id)
    refresh_token = _create_refresh_token(session, user)
    return AuthTokens(
        access_token=access_token,
        refresh_token=refresh_token,
        expires_in=ACCESS_TOKEN_TTL_SECONDS,
    )


@app.post("/api/v1/auth/refresh", response_model=AuthTokens, responses={429: {"model": ErrorEnvelope}})
def refresh_user_token(
    payload: RefreshRequest,
    request: Request,
    session: Session = Depends(get_session),
) -> AuthTokens:
    _check_auth_rate_limit(request)
    token_hash = hashlib.sha256(payload.refresh_token.encode("utf-8")).hexdigest()
    record = session.execute(select(RefreshToken).where(RefreshToken.token_hash == token_hash)).scalar_one_or_none()
    if record is None or record.revoked or record.expires_at.replace(tzinfo=timezone.utc) < _now_utc():
        raise _error(status.HTTP_401_UNAUTHORIZED, "invalid_refresh_token", "Refresh token is invalid")

    user = session.execute(select(User).where(User.id == record.user_id)).scalar_one_or_none()
    if user is None:
        raise _error(status.HTTP_401_UNAUTHORIZED, "unauthorized", "Unknown user")

    record.revoked = True
    access_token = _encode_access_token(user.user_id)
    new_refresh = _create_refresh_token(session, user)
    session.execute(
        delete(RefreshToken)
        .where(RefreshToken.expires_at < (_now_utc() - timedelta(days=1)))
        .execution_options(synchronize_session=False)
    )
    return AuthTokens(
        access_token=access_token,
        refresh_token=new_refresh,
        expires_in=ACCESS_TOKEN_TTL_SECONDS,
    )


@app.get("/api/v1/profile", response_model=ProfileEnvelope)
def get_profile(user: User = Depends(require_user_token)) -> ProfileEnvelope:
    profile = _normalize_profile(user.profile, user.user_id)
    if user.avatar_ref and not profile.avatar_ref:
        profile.avatar_ref = user.avatar_ref
    return ProfileEnvelope(user_id=user.user_id, profile=profile)


@app.put("/api/v1/profile", response_model=ProfileEnvelope)
def update_profile(payload: ProfileV1, user: User = Depends(require_user_token)) -> ProfileEnvelope:
    current = _normalize_profile(user.profile, user.user_id)
    merged = payload.copy(update={"avatar_ref": payload.avatar_ref or current.avatar_ref})
    user.profile = merged.json()
    user.avatar_ref = merged.avatar_ref
    return ProfileEnvelope(user_id=user.user_id, profile=merged)


@app.put("/api/v1/profile/avatar", response_model=ProfileEnvelope)
def update_avatar(
    payload: dict[str, str],
    user: User = Depends(require_user_token),
) -> ProfileEnvelope:
    avatar_ref = payload.get("avatar_ref")
    if not avatar_ref or len(avatar_ref) > 256:
        raise _error(status.HTTP_400_BAD_REQUEST, "invalid_request", "avatar_ref must be 1..256 characters")

    profile = _normalize_profile(user.profile, user.user_id)
    profile.avatar_ref = avatar_ref
    user.avatar_ref = avatar_ref
    user.profile = profile.json()
    return ProfileEnvelope(user_id=user.user_id, profile=profile)
