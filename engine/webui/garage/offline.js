'use strict';
(() => {
  const root = document.querySelector('#offline-screen');
  const find = id => document.querySelector('#offline-' + id);
  let stage = 9, mode = 0, selected = 0, allowsBots = true, modes = [], maps = [], details = [], controls = [], players = [], bots = [], catalog = {};
  let focusedControl, refreshFocus, avatar = '';
  let previousStage, hosting = false, dedicated = 0;
  const clean = text => String(text || '').replace(/\^[0-9]/g, '');
  function node(tag, text, className) {
    const element = document.createElement(tag); if (text !== undefined) element.textContent = clean(text); if (className) element.className = className; return element;
  }
  function send(action, index = 0, value = 0) { location.href = 'q3rally-ui://offline/' + action + '/' + index + '/' + value; }
  function imageFor(model) {
    const [vehicle, paint] = (model || '').split('/');
    const car = catalog.vehicles?.[vehicle];
    return car?.paints?.[paint]?.image || Object.values(car?.paints ?? {})[0]?.image;
  }
  function image(source) { const img = node('img'); img.alt = ''; img.src = source; img.addEventListener('error', () => { img.hidden = true; }); return img; }
  function mapDetails() {
    const map = maps[selected];
    find('map-name').textContent = map?.title || 'No compatible tracks';
    find('map-id').textContent = map?.id || 'TRY ANOTHER GAME MODE';
    const source = catalog.mapShots?.['levelshots/' + map?.id];
    const img = find('map-image'); img.hidden = !source; if (source) img.src = source;
    find('map-details').replaceChildren(...details.map(text => node('p', text)));
    find('next').disabled = !map;
  }
  function mapList() {
    const list = find('map-list'), query = find('search').value.toLowerCase(); list.replaceChildren();
    let visible = 0;
    maps.forEach((map, index) => {
      if (!(map.title + ' ' + map.id).toLowerCase().includes(query)) return;
      visible++;
      const button = node('button', undefined, 'secondary offline-map'); button.setAttribute('aria-pressed',String(index === selected));
      const source = catalog.mapShots?.['levelshots/' + map.id]; if (source) button.append(image(source));
      button.append(node('span', map.title)); button.addEventListener('click', () => send(2,index)); list.append(button);
    });
    find('map-count').textContent = visible + ' / ' + maps.length + ' tracks';
    if (!visible) list.append(node('p', maps.length ? 'No tracks match your search.' : 'No tracks support this mode. Choose another game mode.'));
  }
  function controlElement(control) {
    let element;
    if (control.type === 4) {
      element = node('input'); element.type = 'text'; element.maxLength = control.maxchars || 63; element.value = control.text;
    } else if (control.type === 1) {
      element = node('input'); element.type = 'number'; element.min = 0; element.max = Math.min(99999, Math.pow(10, control.maxchars || 3) - 1); element.step = 1; element.value = control.text;
    } else {
      element = node('select'); control.choices.forEach((label,index) => element.add(new Option(clean(label),String(index)))); element.value = control.value;
    }
    element.id = 'offline-control-' + control.index; element.disabled = control.disabled;
    element.addEventListener('focus', () => { focusedControl = element.id; });
    element.addEventListener('change', () => {
      if (!element.checkValidity()) { element.reportValidity(); return; }
      refreshFocus = focusedControl;
      if (control.type === 4) location.href = 'q3rally-ui://offline/text/' + control.index + '/' + encodeURIComponent(element.value || 'Q3Rally');
      else send(4,control.index, Number(element.value) || 0);
    });
    const label = node('label', control.label.replace(/:\s*$/, '').trim()); label.htmlFor = element.id; label.append(element); return label;
  }
  function options() {
    find('race-title').textContent = maps[selected]?.title || 'Your race'; find('race-mode').textContent = modes[mode] || '';
    const options = find('options'); options.replaceChildren();
    controls.filter(control => control.slot < 0).forEach(control => options.append(controlElement(control)));
    const grid = find('players'); grid.replaceChildren();
    for (let slot = 0; slot < 12; slot++) {
      const rowControls = controls.filter(control => control.slot === slot);
      if (!rowControls.length && slot !== 0) continue;
      if (!allowsBots && slot !== 0) continue;
      const type = rowControls.find(control => !control.team), team = rowControls.find(control => control.team);
      const row = node('div', undefined, 'offline-player'); row.append(node('p', slot === 0 ? (dedicated ? 'Server slot' : 'You / ' + players[slot]) : 'Slot ' + (slot + 1), 'label'));
      if (slot === 0 && !dedicated) {
        if (catalog.avatars?.[avatar]) row.append(image(catalog.avatars[avatar]));
      } else if (type) row.append(controlElement(type));
      if (slot > 0 && type?.value === 1 && allowsBots) {
        const select = node('select'); select.setAttribute('aria-label','Driver for slot ' + (slot + 1));
        bots.slice().sort((a,b) => a.name.localeCompare(b.name)).forEach(bot => select.add(new Option(clean(bot.name),String(bot.index))));
        const current = bots.find(bot => clean(bot.name).toLowerCase() === clean(players[slot]).toLowerCase());
        if (current) select.value = String(current.index); else { select.add(new Option(players[slot] || 'Choose a rival','')); select.value = ''; }
        select.addEventListener('change', () => { if (select.value !== '') send(5,slot,Number(select.value)); });
        const source = imageFor(current?.model); if (source) row.append(image(source));
        row.append(select);
      }
      if (team) row.append(controlElement(team));
      grid.append(row);
    }
    find('grid-help').textContent = allowsBots ? 'Choose your opponents and teams.' : 'This mode is driven solo. Bots are disabled.';
    if (refreshFocus) {
      const element = document.getElementById(refreshFocus);
      element?.focus({preventScroll:true}); refreshFocus = undefined;
    }
  }
  function render() {
    root.classList.toggle('track-selection', stage === 9);
    const modeSelect = find('mode'); modeSelect.replaceChildren(...modes.map((title,index) => new Option(title,String(index)))); modeSelect.value = mode;
    find('track').hidden = stage !== 9; find('rules').hidden = stage !== 10;
    find('step-map').setAttribute('aria-current',stage === 9 ? 'step' : 'false'); find('step-rules').setAttribute('aria-current',stage === 10 ? 'step' : 'false');
    find('back').textContent = stage === 9 ? (hosting ? 'Back to server browser' : 'Back to main menu') : 'Back to tracks';
    root.querySelector('header .label').textContent = hosting ? 'Q3RALLY / HOST A SERVER' : 'Q3RALLY / OFFLINE';
    find('start').textContent = hosting ? 'Start server' : 'Start race';
    if (stage === 9) { mapList(); mapDetails(); } else options();
    find('start').disabled = !maps[selected]; find('status').textContent = '';
  }
  find('mode').addEventListener('change', event => { find('search').value = ''; send(1,Number(event.target.value)); });
  find('search').addEventListener('input',mapList);
  find('back').addEventListener('click', () => send(0));
  find('next').addEventListener('click', () => send(3));
  find('start').addEventListener('click', () => {
    const invalid = [...find('options').querySelectorAll('input')].find(input => !input.checkValidity());
    if (invalid) { invalid.reportValidity(); return; }
    find('start').disabled = true; find('status').textContent = 'Starting your race...'; send(6);
  });
  window.q3rallyOffline = {
    begin(nextStage,nextMode,nextSelected,botsAllowed,isHosting = 0,dedicatedMode = 0) { hosting = !!isHosting; dedicated = dedicatedMode; stage = nextStage; mode = nextMode; selected = nextSelected; allowsBots = !!botsAllowed; modes = []; maps = []; details = []; controls = []; players = []; bots = []; },
    mode(index,title) { modes[index] = title; },
    map(index,id,title) { maps[index] = {id,title}; },
    detail(text) { details.push(text); },
    control(index,type,label,text,value,disabled,slot,team,maxchars) { controls.push({index,type,label,text,value,disabled:!!disabled,slot,team:!!team,maxchars,choices:[]}); },
    choice(index,value,title) { controls.find(control => control.index === index)?.choices.push(title); },
    player(index,name) { players[index] = name; },
    bot(index,name,model) { bots.push({index,name,model}); },
    avatar(path) { avatar = path; },
    end() { render(); if (stage !== previousStage) window.scrollTo(0,0); previousStage = stage; },
    show(screen) { root.hidden = screen !== 9 && screen !== 10; }
  };
  fetch('./catalog.json').then(response => response.json()).then(value => { catalog = value; if (!root.hidden) render(); }).catch(() => {});
})();
