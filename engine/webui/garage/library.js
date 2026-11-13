'use strict';

(() => {
  const screen = document.querySelector('#library-screen');
  const search = document.querySelector('#library-search');
  const type = document.querySelector('#download-type');
  const list = document.querySelector('#library-list');
  const title = document.querySelector('#library-name');
  const image = document.querySelector('#library-image');
  const host = document.querySelector('#rival-3d');
  const install = document.querySelector('#download-install');
  const detailStatus = document.querySelector('#library-detail-status');
  const entries = [];
  let mode;
  let selected = -1;
  let selectedKey;
  let state = 0;
  let catalog;
  let viewer;
  let generation = 0;
  let previewRetry = 0;

  fetch('./catalog.json').then(response => response.json()).then(data => {
    catalog = data;
    if (mode === 5 && !screen.hidden) renderList();
  }).catch(() => { /* Text profiles and downloads also work without showroom assets. */ });

  function busy() { return state === 1 || state === 3; }
  function action(id) {
    location.href = 'q3rally-ui://catalog/action/' + id + '/' + selected;
  }
  function stopViewer() {
    generation++;
    viewer?.dispose();
    viewer = undefined;
    host.hidden = true;
  }
  function carSetup(entry) {
    const [model, paint] = entry.fields[1].split('/');
    const car = catalog?.vehicles[model];
    if (!car) return;
    const skin = car.paints[paint] ? paint : Object.keys(car.paints)[0];
    return {model, skin, car};
  }
  async function showRival(entry) {
    const setup = carSetup(entry);
    const version = generation;
    image.hidden = true;
    if (!setup) return;
    image.src = setup.car.paints[setup.skin].image;
    image.alt = entry.fields[0] + ' / ' + setup.car.name;
    if (!window.Q3RallyShowroom) {
      try { await image.decode(); } catch { return; }
      if (version === generation) image.hidden = false;
      return;
    }
    host.hidden = false;
    host.style.visibility = 'hidden';
    detailStatus.textContent = 'Loading vehicle preview…';
    try {
      const next = await window.Q3RallyShowroom.openVehicleShowroom(
        host, './showroom/', setup.model, setup.car.name, {yawDegrees: 135, distance: 2.1});
      if (version !== generation) { next.dispose(); return; }
      viewer = next;
      await next.update(setup.skin, catalog.wheels.svt_cobra ? 'svt_cobra' : Object.keys(catalog.wheels)[0]);
      if (version !== generation) return;
      image.hidden = true;
      host.style.visibility = 'visible';
      next.show();
      detailStatus.textContent = 'Drag to rotate / scroll to zoom';
    } catch {
      if (version !== generation) return;
      try { await image.decode(); } catch { return; }
      if (version !== generation) return;
      stopViewer();
      image.hidden = false;
      detailStatus.textContent = 'Vehicle preview';
    }
  }
  function downloadPreview(entry) {
    const id = entry.fields[4];
    previewRetry = Date.now();
    if (!/^[a-zA-Z0-9_/.-]+$/.test(id) || id.includes('..')) { image.hidden = true; return; }
    const version = generation;
    const extensions = ['.jpg', '.png', '.jpeg', '.webp', '.gif'];
    let extension = 0;
    image.onload = () => { if (version === generation) image.hidden = false; };
    image.onerror = () => {
      if (version !== generation) return;
      extension++;
      if (extension < extensions.length) image.src = './download-preview/' + id + extensions[extension];
      else image.hidden = true;
    };
    image.alt = entry.fields[0];
    image.src = './download-preview/' + id + extensions[0];
  }
  function favoriteWeapon(entry) {
    const target = document.querySelector('#library-weapon');
    target.replaceChildren();
    const name = mode === 5 && entry ? cleanWeapon(entry.fields[4]) : '';
    target.hidden = !name;
    if (!name) return;
    const icons = {gauntlet:'gauntlet',chainsaw:'gauntlet',machinegun:'machinegun',shotgun:'shotgun',
      grenadelauncher:'grenade',rocketlauncher:'rocket',lightning:'lightning',railgun:'railgun',
      plasmagun:'plasma',bfg10k:'bfg',flamethrower:'flame'};
    const key = name.toLowerCase().replace(/[\s_-]/g,'').replace(/^weapon/,'');
    const source = catalog?.weaponIcons?.['icons/iconw_' + icons[key]];
    if (source) {
      const icon = document.createElement('img');icon.src = source;icon.alt = '';
      icon.width = icon.height = 40;
      icon.addEventListener('error',()=>{icon.hidden=true;});target.append(icon);
    }
    const text = document.createElement('span');text.textContent = 'Favorite weapon / ' + name;
    target.append(text);
  }
  function cleanWeapon(value) { return String(value || '').replace(/\^[0-9]/g,'').trim(); }
  function select(index) {
    stopViewer();
    selected = index;
    const entry = entries[index];
    selectedKey = entry && (mode === 5 ? entry.fields[0] : entry.fields[4]);
    image.hidden = true;
    detailStatus.textContent = '';
    for (const button of list.querySelectorAll('button')) {
      button.setAttribute('aria-pressed', String(Number(button.dataset.index) === index));
    }
    title.textContent = entry?.fields[0] ?? (mode === 5 ? 'No rivals found' : 'Select content');
    document.querySelector('#library-author').textContent = mode === 6 && entry ? 'By ' + entry.fields[1] : '';
    document.querySelector('#library-personality').textContent = mode === 5 && entry ? entry.fields[3] : '';
    favoriteWeapon(entry);
    document.querySelector('#library-description').textContent = mode === 5 && entry ? entry.fields[2] : '';
    if (entry) {
      if (mode === 5) void showRival(entry);
      else {
        detailStatus.textContent = [entry.fields[2], entry.fields[3], (entry.size / 1024).toFixed(1) + ' MB',
          entry.installed ? 'Installed' : 'Available'].filter(Boolean).join(' / ');
        downloadPreview(entry);
      }
    }
    install.disabled = busy() || !entry || entry.installed;
  }
  function renderList() {
    const query = search.value.trim().toLowerCase();
    const matching = entries.filter(entry =>
      (mode !== 6 || !type.value || entry.fields[2] === type.value) &&
      entry.fields.slice(0, mode === 5 ? 4 : 3).join(' ').toLowerCase().includes(query));
    list.replaceChildren(...matching.map(entry => {
      const button = document.createElement('button');
      button.type = 'button';
      button.className = 'secondary';
      button.dataset.index = entry.index;
      if (mode === 5) {
        const setup = carSetup(entry);
        if (setup) {
          const icon = document.createElement('img');
          icon.src = setup.car.paints[setup.skin].image;
          icon.alt = '';
          button.append(icon);
        }
      }
      const name = document.createElement('span');
      name.textContent = entry.fields[0] + (mode === 6 && entry.installed ? ' / Installed' : '');
      button.append(name);
      button.addEventListener('click', () => select(entry.index));
      return button;
    }));
    document.querySelector('#library-count').textContent = matching.length + (mode === 5 ? ' rivals' : ' items');
    const previous = matching.find(entry => (mode === 5 ? entry.fields[0] : entry.fields[4]) === selectedKey);
    select((previous ?? matching[0])?.index ?? -1);
  }

  window.q3rallyLibrary = {
    show(nextMode) {
      const entering = screen.hidden;
      screen.hidden = nextMode !== 5 && nextMode !== 6;
      if (screen.hidden) { if (viewer || !host.hidden) stopViewer(); return; }
      mode = nextMode;
      document.querySelector('#library-title').innerHTML = mode === 5 ? 'Meet your <em>rivals.</em>' : 'Expand your <em>rally.</em>';
      document.querySelector('#library-label').textContent = mode === 5 ? 'Q3RALLY / RIVALS' : 'Q3RALLY / DOWNLOADS';
      document.querySelector('#library-detail-label').textContent = mode === 5 ? 'RIVAL PROFILE' : 'CONTENT DETAILS';
      document.querySelector('#download-filters').hidden = mode !== 6;
      document.querySelector('#download-controls').hidden = mode !== 6;
      search.placeholder = mode === 5 ? 'Find a rival' : 'Find content';
      if (entering) renderList();
    },
    begin(nextMode) {
      if (mode !== nextMode) { search.value = ''; type.value = ''; selectedKey = undefined; }
      mode = nextMode;
      entries.length = 0;
    },
    item(itemMode, index, fields, size, installed) {
      if (itemMode === mode) entries[index] = {index, fields, size, installed: !!installed};
    },
    end(itemMode) { if (itemMode === mode) renderList(); },
    downloadState(nextState, progress, filename, error) {
      state = nextState;
      document.querySelector('#download-progress').value = Math.max(0, Math.min(100, progress));
      const messages = ['Choose content to install.', 'Fetching catalog...', 'Catalog ready.',
        'Downloading ' + filename + ' / ' + progress + '%', 'Installed. Restart Q3Rally to use the new content.', error || 'Download failed. Try refreshing the catalog.'];
      document.querySelector('#download-status').textContent = messages[state] ?? 'Waiting for download service.';
      document.querySelector('#download-refresh').disabled = busy();
      document.querySelector('#download-cancel').disabled = !busy();
      install.disabled = busy() || selected < 0 || !!entries[selected]?.installed;
      if (mode === 6 && !screen.hidden && entries[selected] && image.hidden && Date.now() - previewRetry > 2000)
        downloadPreview(entries[selected]);
    },
  };
  search.addEventListener('input', renderList);
  type.addEventListener('change', renderList);
  document.querySelector('#library-back').addEventListener('click', () => action(0));
  document.querySelector('#download-refresh').addEventListener('click', () => action(1));
  document.querySelector('#download-cancel').addEventListener('click', () => action(2));
  install.addEventListener('click', () => action(3));
  document.addEventListener('keydown', event => {
    if (!screen.hidden && event.key === 'Escape') {
      event.preventDefault();
      action(0);
    }
  });
})();
