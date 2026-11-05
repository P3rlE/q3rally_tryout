'use strict';
(() => {
  const countries = [["", "Not specified"], ["AT", "Austria"], ["AU", "Australia"], ["BE", "Belgium"], ["BR", "Brazil"], ["CA", "Canada"], ["CH", "Switzerland"], ["CN", "China"], ["CZ", "Czech Republic"], ["DE", "Germany"], ["DK", "Denmark"], ["ES", "Spain"], ["FI", "Finland"], ["FR", "France"], ["GB", "United Kingdom"], ["GR", "Greece"], ["HU", "Hungary"], ["ID", "Indonesia"], ["IN", "India"], ["IT", "Italy"], ["JP", "Japan"], ["KR", "South Korea"], ["MX", "Mexico"], ["NL", "Netherlands"], ["NO", "Norway"], ["NZ", "New Zealand"], ["PL", "Poland"], ["PT", "Portugal"], ["RO", "Romania"], ["RU", "Russia"], ["SE", "Sweden"], ["SG", "Singapore"], ["SK", "Slovakia"], ["TH", "Thailand"], ["TR", "Turkey"], ["TW", "Taiwan"], ["UA", "Ukraine"], ["US", "United States"], ["VN", "Vietnam"], ["ZA", "South Africa"], ["AR", "Argentina"], ["CL", "Chile"]];
  const root = document.querySelector('#driver-screen');
  const panels = ['overview', 'stats', 'achievements'].map(id => document.querySelector('#driver-' + id));
  let profile, available = [], pending = false, categories = [], rank = [0, 0, 0], catalog = {};
  const number = value => Number.isFinite(Number(value)) ? Number(value) : 0;
  const format = value => number(value).toLocaleString('en-US', { maximumFractionDigits: 1 });
  const node = (tag, text, className) => {
    const element = document.createElement(tag);
    if (text !== undefined) element.textContent = text;
    if (className) element.className = className;
    return element;
  };
  function action(label, command) {
    const button = node('button', label, 'secondary');
    button.addEventListener('click', () => {
      if (command === 'edit' || command === 'switch') openEditor(command);
      else location.href = 'q3rally-ui://driver/' + command;
    });
    return button;
  }
  const dialog = node('dialog'); dialog.id = 'driver-dialog';
  const dialogStatus = node('p', '', 'driver-dialog-status'); dialogStatus.setAttribute('role', 'status');
  document.body.append(dialog);
  let dialogMode, selectedAvatar;
  function field(form, title, input) {
    const label = node('label', title); label.append(input); form.append(label); return input;
  }
  function input(type, value, name) {
    const element = node('input'); element.type = type; element.name = name; element.value = value || ''; return element;
  }
  function submit(form, title, command, payload) {
    const button = node('button', title); button.type = 'submit'; form.append(button);
    form.addEventListener('submit', event => {
      event.preventDefault();
      if (pending || !form.reportValidity()) return;
      pending = true; dialogStatus.textContent = 'Saving...';
      dialog.querySelectorAll('button, input, select').forEach(control => { control.disabled = true; });
      location.href = 'q3rally-ui://driver/' + command + '/' + encodeURIComponent(JSON.stringify(payload()));
    });
  }
  function openEditor(mode) {
    dialogMode = mode; pending = false; dialog.replaceChildren();
    dialog.setAttribute('aria-label', mode === 'edit' ? 'Edit driver' : 'Select or create profile');
    dialog.append(node('p', 'Q3RALLY / YOUR IDENTITY', 'label'), node('h2', mode === 'edit' ? 'Edit driver' : 'Your drivers'));
    const close = node('button', 'Close', 'secondary'); close.type = 'button'; close.addEventListener('click', () => dialog.close());
    dialogStatus.textContent = '';
    if (mode === 'edit') {
      const info = profile?.identity?.info ?? {};
      const form = node('form', undefined, 'driver-form');
      form.append(node('p', 'Profile: ' + (profile?.identity?.name ?? '')));
      const gender = node('select'); gender.name = 'gender';
      ['', 'Female', 'Male', 'Non-binary', 'Other'].forEach(value => gender.add(new Option(value || 'Unspecified', value)));
      gender.value = info.gender || ''; field(form, 'Gender (optional)', gender);
      const birth = field(form, 'Birth date (optional)', input('date', info.birthDate, 'birthDate')); birth.min = '1900-01-01'; birth.max = new Date().toISOString().slice(0,10);
      const country = node('select'); country.name = 'country';
      countries.slice().sort((a,b) => !a[0] ? -1 : !b[0] ? 1 : a[1].localeCompare(b[1])).forEach(([code, title]) => country.add(new Option(title, code)));
      if (info.country && !countries.some(([code]) => code === info.country)) country.add(new Option(info.country, info.country));
      country.value = info.country || ''; field(form, 'Country (optional)', country);
      form.append(node('p', 'Avatar', 'label'));
      const choices = node('div', undefined, 'driver-avatar-choices'); choices.setAttribute('role','group'); choices.setAttribute('aria-label','Avatar');
      selectedAvatar = info.avatar || Object.keys(catalog.avatars ?? {})[0] || '';
      Object.entries(catalog.avatars ?? {}).forEach(([key, source]) => {
        const button = node('button', undefined, 'secondary'); button.type = 'button'; button.title = key.split('/').pop().replace(/_/g,' '); button.setAttribute('aria-label',button.title);
        button.setAttribute('aria-pressed', String(key === selectedAvatar));
        const image = node('img'); image.src = source; image.alt = ''; button.append(image);
        button.addEventListener('click', () => { selectedAvatar = key; choices.querySelectorAll('button').forEach(b => b.setAttribute('aria-pressed', String(b === button))); });
        choices.append(button);
      });
      form.append(choices);
      submit(form, 'Save driver', 'save', () => ({uuid:profile?.identity?.uuid, gender:gender.value, birthDate:birth.value, country:country.value.toUpperCase(), avatar:selectedAvatar}));
      dialog.append(form);
    } else {
      const list = node('div', undefined, 'driver-profile-list');
      available.slice().sort((a,b)=>a.localeCompare(b)).forEach(name => {
        const button = node('button', name + (name === profile?.identity?.name ? ' \u00b7 Active' : ''), 'secondary');
        button.addEventListener('click', () => {
          if (pending) return; pending = true; dialogStatus.textContent = 'Selecting profile...';
          dialog.querySelectorAll('button, input').forEach(control => { control.disabled = true; });
          location.href = 'q3rally-ui://driver/select/' + encodeURIComponent(JSON.stringify({name}));
        }); list.append(button);
      });
      if (!available.length) list.append(node('p', 'No local profiles yet. Create your first driver below.'));
      dialog.append(list);
      const form = node('form', undefined, 'driver-form'); form.append(node('h3', 'Create a driver'));
      const name = field(form, 'Profile name', input('text', '', 'name')); name.required = true; name.maxLength = 31; name.pattern = '(?:[A-Za-z0-9_]|-)+'; name.autocomplete = 'off';
      form.append(node('p', 'Use letters, numbers, a dash or an underscore. A new profile starts with its own progress.'));
      submit(form, 'Create & select', 'create', () => ({name:name.value})); dialog.append(form);
    }
    dialog.append(dialogStatus, close);
    if (!dialog.open) dialog.showModal();
  }
  dialog.addEventListener('cancel', event => { if (pending) event.preventDefault(); });
  function tab(index) {
    panels.forEach((panel, i) => { panel.hidden = i !== index; });
    root.querySelectorAll('[data-driver-tab]').forEach((button, i) => {
      button.setAttribute('aria-selected', String(i === index)); button.tabIndex = i === index ? 0 : -1;
    });
  }
  root.querySelectorAll('[data-driver-tab]').forEach((button, index, buttons) => {
    button.addEventListener('click', () => tab(index));
    button.addEventListener('keydown', event => {
      let next;
      if (event.key === 'ArrowRight') next = (index + 1) % 3;
      if (event.key === 'ArrowLeft') next = (index + 2) % 3;
      if (event.key === 'Home') next = 0;
      if (event.key === 'End') next = 2;
      if (next !== undefined) { event.preventDefault(); tab(next); buttons[next].focus(); }
    });
  });
  root.querySelector('[data-driver-action="back"]').addEventListener('click', () => { location.href = 'q3rally-ui://driver/back'; });
  const stats = () => profile?.progress?.stats ?? {};
  function cards(parent, entries) {
    const grid = node('div', undefined, 'driver-metrics');
    entries.forEach(([label, value]) => { const card = node('div'); card.append(node('p', label, 'label'), node('strong', value)); grid.append(card); });
    parent.append(grid);
  }
  function renderOverview() {
    const target = panels[0]; target.replaceChildren();
    const identity = profile?.identity, info = identity?.info ?? {}, values = stats();
    const section = node('section', undefined, 'driver-identity');
    const avatar = catalog.avatars?.[info.avatar];
    if (avatar) { const image = node('img'); image.src = avatar; image.alt = 'Driver avatar'; section.append(image); }
    const details = node('div');
    details.append(node('p', 'YOUR DRIVER', 'label'), node('h2', identity?.name?.replace(/\^[0-9]/g, '') || 'Choose your driver'));
    details.append(node('p', identity ? (catalog.ranks?.[rank[0]] ?? 'Driver') : 'Create or select a profile to begin.'));
    if (info.country) details.append(node('p', 'Country: ' + (countries.find(([code]) => code === info.country)?.[1] ?? info.country)));
    section.append(details); target.append(section);
    const career = node('section'); career.append(node('h2', 'Your progression'));
    cards(career, [['Profile points', format(values.playerScore)], ['Games played', format(values.gamesPlayed)], ['Wins', format(values.wins)], ['Distance', format(values.distanceKm) + ' km']]);
    const progress = node('progress'); progress.max = 100;
    progress.value = !identity ? 0 : rank[2] > rank[1] ? Math.max(0, Math.min(100, (number(values.playerScore) - rank[1]) / (rank[2] - rank[1]) * 100)) : 100;
    progress.setAttribute('aria-label', 'Progress to next rank'); career.append(progress);
    career.append(node('p', !identity ? 'No active profile' : rank[2] > rank[1] ? format(Math.max(0, rank[2] - number(values.playerScore))) + ' points to the next rank' : 'Highest rank reached'));
    const actions = node('div', undefined, 'driver-actions'); const edit = action('Edit driver', 'edit'); edit.disabled = !identity;
    actions.append(edit, action('Select / create profile', 'switch'), action('Web profile & garage', 'garage'));
    career.append(actions); target.append(career);
  }
  const groups = {
    General: ['gamesPlayed', 'wins', 'losses', 'playerScore', 'distanceKm', 'fuelUsed', 'topSpeedKph', 'bestLapMs', 'kills', 'deaths', 'damageDealt', 'damageTaken', 'flagCaptures', 'flagAssists', 'accuracyAwards', 'excellentAwards', 'impressiveAwards', 'perfectAwards', 'mostUsedVehicle', 'mostUsedVehicleTimeMs'],
    Racing: ['racingWins', 'racingPodiums', 'racingCompleted', 'racingTotalMs'],
    'Racing deathmatch': ['racingDmWins', 'racingDmPodiums', 'racingDmCompleted', 'racingDmTotalMs'],
    Sprint: ['sprintWins', 'sprintCompleted', 'sprintBestMs'],
    Elimination: ['eliminationWins', 'eliminationCompleted', 'eliminationTotalRoundsLasted'],
    'Last car standing': ['lcsWins', 'lcsCompleted', 'lcsTotalSurvivalMs'],
    Derby: ['derbyWins', 'derbyCompleted', 'derbyKills'],
    Deathmatch: ['dmWins', 'dmCompleted', 'dmKills'],
    'Capture the flag': ['ctfWins', 'ctfCompleted', 'ctfCaptures'],
    'Four-team CTF': ['ctf4Wins', 'ctf4Completed', 'ctf4Captures'],
    'Team deathmatch': ['teamWins', 'teamCompleted', 'teamKills'],
    'Team racing': ['teamRacingWins', 'teamRacingCompleted', 'teamRacingPodiums'],
    'Team racing deathmatch': ['teamRacingDmWins', 'teamRacingDmCompleted', 'teamRacingDmPodiums'],
    Domination: ['dominationWins', 'dominationCompleted', 'dominationZoneHoldMs'],
    'King of the hill': ['kothWins', 'kothCompleted', 'kothZoneHoldMs'],
    Autoball: ['autoballWins', 'autoballCompleted', 'autoballGoals']
  };
  function statValue(key) {
    const value = stats()[key];
    if (key === 'mostUsedVehicle') return value || 'None yet';
    if (key.endsWith('Ms')) {
      const ms = Math.max(0, number(value));
      if (!ms) return '\u2014';
      return Math.floor(ms / 60000) + ':' + String(Math.floor(ms / 1000) % 60).padStart(2, '0') + '.' + String(Math.floor(ms % 1000)).padStart(3, '0');
    }
    return format(value) + ({distanceKm: ' km', fuelUsed: ' l', topSpeedKph: ' km/h'}[key] ?? '');
  }
  function renderStats() {
    panels[1].replaceChildren();
    Object.entries(groups).forEach(([name, keys]) => {
      const section = node('section'); section.append(node('h2', name));
      cards(section, keys.map(key => [key.replace(/([A-Z])/g, ' $1').replace(/ Ms$/, '').replace(/^./, c => c.toUpperCase()), statValue(key)]));
      panels[1].append(section);
    });
  }
  function renderMedals() {
    const selected = document.querySelector('#driver-achievement-filter').value;
    const target = document.querySelector('#driver-medals'); target.replaceChildren();
    let earned = 0, total = 0;
    categories.forEach((category, index) => {
      category.tiers.forEach(tier => {
        total++; if (tier.earned) earned++;
        if (selected !== '' && number(selected) !== index) return;
        const card = node('article', undefined, 'driver-medal' + (tier.earned ? ' earned' : ' locked'));
        const source = catalog.achievementIcons?.[tier.icon];
        if (source) { const image = node('img'); image.src = source; image.alt = ''; card.append(image); }
        card.append(node('p', category.title, 'label'), node('h3', tier.name), node('p', tier.description));
        const progress = node('progress'); progress.max = Math.max(1, tier.threshold); progress.value = Math.max(0, Math.min(tier.threshold, category.progress));
        progress.setAttribute('aria-label', category.title + ': ' + tier.name);
        card.append(progress, node('p', tier.earned ? 'Earned' : 'Locked \u00b7 ' + format(category.progress) + ' / ' + format(tier.threshold)));
        target.append(card);
      });
    });
    document.querySelector('#driver-achievement-count').textContent = earned + ' / ' + total + ' achievements earned';
  }
  document.querySelector('#driver-achievement-filter').addEventListener('change', renderMedals);
  function render() { renderOverview(); renderStats(); renderMedals(); }
  window.q3rallyDriver = {
    begin() { profile = undefined; available = []; categories = []; },
    available(name) { if (!available.includes(name)) available.push(name); },
    result(message, ok) {
      pending = false;
      if (ok) { openEditor(dialogMode); }
      dialog.querySelectorAll("button, input, select").forEach(control => { control.disabled = false; });
      dialogStatus.textContent = message;
    },
    profile(value) { profile = value; },
    rank(...value) { rank = value; },
    category(index, title, progress) { categories[index] = {title, progress: number(progress), tiers: []}; },
    tier(index, name, description, icon, threshold, earned) { categories[index]?.tiers.push({name, description, icon, threshold: number(threshold), earned: earned === '1'}); },
    end(initialTab) {
      const select = document.querySelector('#driver-achievement-filter'); select.replaceChildren(new Option('All achievements', ''));
      categories.forEach((category, index) => select.add(new Option(category.title, String(index))));
      render(); tab(Math.max(0, Math.min(2, number(initialTab))));
    },
    show(screen) { root.hidden = screen !== 8; }
  };
  fetch('./catalog.json').then(response => response.json()).then(value => { catalog = value; render(); }).catch(() => {});
})();
