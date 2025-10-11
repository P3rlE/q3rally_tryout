<?php
// router.php

// The document root is where this router script is.
$docRoot = __DIR__;

// The requested URI, stripped of query string.
$uri = parse_url($_SERVER['REQUEST_URI'], PHP_URL_PATH);

// The path to the requested resource on the filesystem.
$path = $docRoot . $uri;

// If the requested resource is a file and it exists, serve it directly.
if ($uri !== '/' && file_exists($path) && is_file($path)) {
    return false;
}

// Otherwise, we route the request to our main index.php script.
// This allows for clean URLs and a single entry point for the application.
require_once $docRoot . '/index.php';
