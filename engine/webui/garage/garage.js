'use strict';

const image = document.querySelector('#vehicle-image');
const title = document.querySelector('#vehicle-name');
const status = document.querySelector('#status');
const vehicle = document.querySelector('#vehicle');
const canvasHost = document.querySelector('#vehicle-3d');
const toggle = document.querySelector('#view-toggle');
const reset = document.querySelector('#view-reset');
const viewStatus = document.querySelector('#view-status');
const paint = document.querySelector('#paint');
const wheel = document.querySelector('#wheel');
const head = document.querySelector('#head');
const plate = document.querySelector('#plate');
const favoriteSlots = document.querySelector('#favorite-slots');
const favorites = new Map();
let pendingSetup;
const controls = document.querySelector('#setup-controls');
const apply = document.querySelector('#apply');
const setups = new Map();
let catalog;
let unlockedRank = -1;
let initialSetup;
let showroom;
let showroomVehicle;
let use3d = false;
let generation = 0;
let textureGeneration = 0;

const carouselLabels = new Map([[vehicle, 'vehicle'], [paint, 'paint'], [wheel, 'wheels'], [head, 'driver'], [plate, 'license plate']]);
const carousels = [vehicle, paint, wheel, head, plate].map(createCarousel);

function carouselEntry(field, id) {
  if (!catalog) return;
  if (field === vehicle) {
    const car = catalog.vehicles[id];
    const skin = car.paints[setups.get(id)?.paint] ?? Object.values(car.paints).find(option => !isLocked(option)) ?? Object.values(car.paints)[0];
    return {...car, image: skin?.image};
  }
  if (field === paint) return catalog.vehicles[selected()]?.paints[id];
  return (field === wheel ? catalog.wheels : field === head ? catalog.heads : catalog.plates)?.[id];
}

function createCarousel(field) {
  const group = field.parentElement.querySelector('.garage-carousel');
  const label = carouselLabels.get(field);
  const previous = document.createElement('button');
  const next = document.createElement('button');
  const before = document.createElement('button');
  const after = document.createElement('button');
  const current = document.createElement('div');
  previous.type = next.type = before.type = after.type = 'button';
  previous.className = next.className = 'carousel-arrow secondary';
  before.className = after.className = 'carousel-neighbor secondary';
  current.className = 'carousel-current';
  current.tabIndex = 0;
  current.setAttribute('aria-live', 'polite');
  previous.textContent = '\u2039';
  next.textContent = '\u203a';
  previous.setAttribute('aria-label', 'Previous ' + label);
  next.setAttribute('aria-label', 'Next ' + label);

  function move(offset) {
    if (field.disabled || field.options.length < 2) return;
    field.selectedIndex = (field.selectedIndex + offset + field.options.length) % field.options.length;
    field.dispatchEvent(new Event('change', {bubbles: true}));
  }
  previous.addEventListener('click', () => move(-1));
  before.addEventListener('click', () => move(-1));
  next.addEventListener('click', () => move(1));
  after.addEventListener('click', () => move(1));
  group.addEventListener('keydown', event => {
    if (event.altKey || event.ctrlKey || event.metaKey) return;
    if (event.key === 'ArrowLeft' || event.key === 'ArrowRight') {
      event.preventDefault();
      move(event.key === 'ArrowLeft' ? -1 : 1);
    }
  });
  group.append(previous, before, current, after, next);

  function card(node, index, active) {
    const option = field.options[index];
    const entry = option && carouselEntry(field, option.value);
    const name = document.createElement('span');
    name.className = 'carousel-name';
    name.textContent = entry?.name ?? 'Loading...';
    const children = [];
    if (entry?.image) {
      const thumbnail = document.createElement('img');
      thumbnail.src = entry.image;
      thumbnail.alt = '';
      children.push(thumbnail);
    }
    children.push(name);
    if (entry && isLocked(entry)) {
      const lock = document.createElement('span');
      lock.className = 'carousel-lock';
      lock.textContent = 'Locked';
      children.push(lock);
    }
    if (active) {
      const count = document.createElement('span');
      count.className = 'carousel-count';
      count.textContent = field.options.length ? (index + 1) + ' / ' + field.options.length : '';
      children.push(count);
    } else {
      node.setAttribute('aria-label', label + ': ' + (entry?.name ?? 'Loading') + (entry && isLocked(entry) ? ' (Locked)' : ''));
    }
    node.replaceChildren(...children);
    node.classList.toggle('is-locked', !!entry && isLocked(entry));
  }

  return () => {
    const count = field.options.length;
    const index = Math.max(0, field.selectedIndex);
    const disabled = field.disabled || count < 2;
    previous.disabled = next.disabled = before.disabled = after.disabled = disabled;
    before.hidden = after.hidden = count < 3;
    group.classList.toggle('carousel-short', count < 3);
    current.setAttribute('aria-label', label + ' selection');
    card(current, index, true);
    if (count > 2) {
      card(before, (index - 1 + count) % count, false);
      card(after, (index + 1) % count, false);
    }
  };
}

function renderCarousels() {
  for (const render of carousels) render();
}

renderCarousels();

function selected() {
  return vehicle.value;
}

function showImage() {
  canvasHost.hidden = true;
  image.hidden = false;
  reset.hidden = true;
}

function disposeShowroom() {
  textureGeneration++;
  showroom?.dispose();
  showroom = undefined;
  showroomVehicle = undefined;
  showImage();
}

function stop3d() {
  generation++;
  use3d = false;
  disposeShowroom();
  toggle.textContent = '3D view';
  viewStatus.textContent = '';
}

async function update3d() {
  const id = selected();
  const setup = setups.get(id);
  const current = showroom;
  if (!current || showroomVehicle !== id || !setup) return;
  const version = ++textureGeneration;
  try {
    await current.update(setup.paint, setup.wheels);
    if (version !== textureGeneration) return;
    canvasHost.hidden = false;
    image.hidden = true;
    reset.hidden = false;
    viewStatus.textContent = 'Drag to rotate · scroll to zoom';
    current.show();
  } catch {
    if (version !== textureGeneration) return;
    stop3d();
    viewStatus.textContent = '3D preview unavailable. Showing the vehicle image.';
  }
}

async function load3d(id) {
  const current = ++generation;
  disposeShowroom();
  toggle.textContent = 'Photo view';
  viewStatus.textContent = 'Loading 3D preview…';
  try {
    const next = await window.Q3RallyShowroom.openVehicleShowroom(canvasHost, './showroom/', id, catalog.vehicles[id].name);
    if (current !== generation) {
      next.dispose();
      return;
    }
    showroom = next;
    showroomVehicle = id;
    await update3d();
  } catch {
    if (current !== generation) return;
    stop3d();
    viewStatus.textContent = '3D preview unavailable. Showing the vehicle image.';
  }
}

toggle.addEventListener('click', () => {
  if (use3d) stop3d();
  else {
    use3d = true;
    void load3d(selected());
  }
});
reset.addEventListener('click', () => showroom?.reset());

function setOptions(select, entries, value) {
  select.replaceChildren(...Object.entries(entries).map(([id, option]) => new Option(option.name + (isLocked(option) ? ' (Locked)' : ''), id)));
  select.value = value;
}

function refreshImages(id, setup) {
  const selectedPaint = catalog.vehicles[id].paints[setup.paint];
  image.src = selectedPaint.image;
  image.alt = catalog.vehicles[id].name + ' in ' + selectedPaint.name;
  renderCarousels();

}

function isLocked(option) {
  return option.rank > 0 && option.rank > unlockedRank;
}

function refreshStatus(id, setup) {
  const car = catalog.vehicles[id];
  const locked = [car, car.paints[setup.paint]].filter(isLocked);
  apply.disabled = locked.length > 0 || !!pendingSetup;
  status.textContent = locked.length
    ? locked.map(option => option.name + ' unlocks at ' +
        (catalog.ranks[option.rank] ?? 'rank ' + option.rank) + '.').join(' ')
    : 'Preview. Apply setup to use it in Q3Rally.';
}

function refreshVehicle() {
  const id = selected();
  const car = catalog.vehicles[id];
  title.textContent = car.name + (isLocked(car) ? ' (Locked)' : '');
  const paints = car.paints;
  const setup = setups.get(id) ?? {
    paint: Object.keys(paints).find(key => !isLocked(paints[key])) ?? Object.keys(paints)[0],
    wheels: catalog.wheels.svt_cobra ? 'svt_cobra' : Object.keys(catalog.wheels)[0],
    head: catalog.heads?.doom ? 'doom' : Object.keys(catalog.heads ?? {})[0],
    plate: catalog.plates?.usa_california ? 'usa_california' : Object.keys(catalog.plates ?? {})[0],
  };
  setups.set(id, setup);
  setOptions(paint, paints, setup.paint);
  wheel.value = setup.wheels;
  head.value = setup.head;
  plate.value = setup.plate;
  refreshImages(id, setup);
  refreshStatus(id, setup);
  renderFavorites();
  if (use3d) {
    if (showroomVehicle === id) void update3d();
    else void load3d(id);
  }
}

function applyInitialSetup() {
  if (!catalog) return;
  const { id, paint: skin, wheels, head: driver, plate: tag } = initialSetup ?? {};
  const active = catalog.vehicles[id];
  setOptions(vehicle, catalog.vehicles, active ? id : 'alpine');
  if (active) {
    const validPaint = active.paints[skin] ? skin : Object.keys(active.paints).find(key => !isLocked(active.paints[key]));
    const validWheels = catalog.wheels[wheels] ? wheels : wheel.value;
    if (validPaint && validWheels) setups.set(id, { paint: validPaint, wheels: validWheels,
      head: catalog.heads?.[driver] ? driver : head.value,
      plate: catalog.plates?.[tag] ? tag : plate.value });
  }
  refreshVehicle();
}

window.q3rallyGarage = {
  select(id, skin, wheels, rank, driver, tag) {
    unlockedRank = rank;
    initialSetup = { id, paint: skin, wheels, head: driver, plate: tag };
    window.q3rallyFrontend?.vehicle(id, skin, wheels);
    applyInitialSetup();
  },
  cloudStatus(message) {
    document.querySelector('#web-profile-status').textContent = message;
  },
  cloudSetup(id, skin, wheels, driver, tag) {
    if (!catalog || selected() !== id || !catalog.vehicles[id]?.paints[skin] || !catalog.wheels[wheels]) return;
    setups.set(id, {paint: skin, wheels, head: catalog.heads[driver] ? driver : head.value,
      plate: catalog.plates[tag] ? tag : plate.value});
    refreshVehicle();
    status.textContent = 'Web setup loaded for preview. Apply setup to use it in Q3Rally.';
  },
  favorite(slot, id, skin, wheels, driver, tag) {
    favorites.set(slot, id ? {id, paint: skin, wheels, head: driver, plate: tag} : undefined);
    renderFavorites();
  },
  savedFavorite(slot, ok) {
    pendingSetup = undefined;
    if (catalog) refreshStatus(selected(), setups.get(selected()));
    renderFavorites();
    status.textContent = ok ? 'Setup saved in slot ' + (slot + 1) + '.' : 'Could not save the favorite. Check that a driver profile is active.';
  },
  result(ok) {
    const submitted = pendingSetup;
    pendingSetup = undefined;
    if (ok && submitted) window.q3rallyFrontend?.vehicle(submitted.id, submitted.paint, submitted.wheels);
    if (catalog) refreshStatus(selected(), setups.get(selected()));
    renderFavorites();
    status.textContent = ok ? 'Setup applied in Q3Rally.' : 'Setup unavailable in this build. The previous setup was kept.';
  },
};

fetch('./catalog.json').then(response => {
  if (!response.ok) throw new Error('Missing setup catalog');
  return response.json();
}).then(data => {
  catalog = data;
  setOptions(wheel, catalog.wheels, 'svt_cobra');
  setOptions(head, catalog.heads ?? {}, catalog.heads?.doom ? 'doom' : Object.keys(catalog.heads ?? {})[0]);
  setOptions(plate, catalog.plates ?? {}, catalog.plates?.usa_california ? 'usa_california' : Object.keys(catalog.plates ?? {})[0]);
  controls.hidden = false;
  vehicle.disabled = false;
  toggle.hidden = !window.Q3RallyShowroom;
  applyInitialSetup();
}).catch(() => {
  catalog = undefined;
  controls.hidden = true;
  apply.disabled = true;
  toggle.hidden = true;
  status.textContent = 'Local setup catalog unavailable.';
});

vehicle.addEventListener('change', refreshVehicle);
for (const field of [paint, wheel, head, plate]) field.addEventListener('change', () => {
  const id = selected();
  const setup = { paint: paint.value, wheels: wheel.value, head: head.value, plate: plate.value };
  setups.set(id, setup);
  refreshImages(id, setup);
  refreshStatus(id, setup);
  renderFavorites();
  if (use3d) void update3d();
});
function submitSetup(slot) {
  const id = selected();
  const setup = setups.get(id);
  if (!setup || pendingSetup || isLocked(catalog.vehicles[id]) || isLocked(catalog.vehicles[id].paints[setup.paint])) return;
  pendingSetup = {id, ...setup};
  refreshStatus(id, setup);
  renderFavorites();
  const path = [id, setup.paint, setup.wheels, setup.head, setup.plate].join('/');
  location.href = 'q3rally-ui://garage/' + (slot === undefined ? 'apply/' : 'save/' + slot + '/') + path;
}

function renderFavorites() {
  if (!catalog) return;
  favoriteSlots.replaceChildren(...Array.from({length: 4}, (_, slot) => {
    const item = favorites.get(slot);
    const car = item && catalog.vehicles[item.id];
    const skin = car?.paints[item.paint];
    const card = document.createElement('div');
    const label = document.createElement('p');
    label.textContent = 'Slot ' + (slot + 1);
    card.append(label);
    if (skin) {
      const icon = document.createElement('img');
      icon.src = skin.image;
      icon.alt = car.name + ' / ' + skin.name;
      card.append(icon);
    }
    const load = document.createElement('button');
    load.type = 'button';
    load.className = 'secondary';
    load.textContent = car && skin ? car.name + ' / ' + skin.name + (isLocked(car) || isLocked(skin) ? ' (Locked)' : '') : 'Empty';
    load.disabled = !car || !skin;
    load.addEventListener('click', () => {
      setups.set(item.id, {paint: item.paint,
        wheels: catalog.wheels[item.wheels] ? item.wheels : wheel.value,
        head: catalog.heads?.[item.head] ? item.head : head.value,
        plate: catalog.plates?.[item.plate] ? item.plate : plate.value});
      vehicle.value = item.id;
      refreshVehicle();
    });
    const save = document.createElement('button');
    save.type = 'button';
    save.textContent = 'Save here';
    save.disabled = apply.disabled;
    save.addEventListener('click', () => submitSetup(slot));
    const upload = document.createElement('button');
    upload.type = 'button';
    upload.className = 'secondary';
    upload.textContent = 'Upload to web profile';
    upload.disabled = !car || !skin;
    upload.addEventListener('click', () => {
      location.href = 'q3rally-ui://garage/upload/' + slot;
    });
    card.append(load, save, upload);
    return card;
  }));
}
apply.addEventListener('click', () => submitSetup());
document.querySelector('#back').addEventListener('click', () => {
  location.href = 'q3rally-ui://garage/close';
});
document.addEventListener('keydown', event => {
  if (event.key === 'Escape' && !event.repeat && !event.defaultPrevented && !document.querySelector('#garage').hidden) {
    event.preventDefault();
    location.href = 'q3rally-ui://garage/close';
  }
});

document.querySelector('#web-link').addEventListener('click', () => {
  location.href = 'q3rally-ui://garage/link';
});
document.querySelector('#web-pull').addEventListener('click', () => {
  location.href = 'q3rally-ui://garage/sync/' + selected();
});
