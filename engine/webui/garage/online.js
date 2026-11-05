'use strict';
(() => {
  const root = document.querySelector('#online-screen');
  const find = id => document.querySelector('#online-' + id);
  const clean = text => String(text || '').replace(/\^[0-9]/g, '');
  let servers = [], options = {}, filters = [], selected = '', scanning = false, catalog = {}, message = '', pendingRemoval = '';
  function node(tag,text,className) { const element = document.createElement(tag); if(text !== undefined) element.textContent = clean(text); if(className) element.className = className; return element; }
  function send(action,index=0,value=0) { location.href = 'q3rally-ui://online/' + action + '/' + index + '/' + value; }
  function addressAction(action,address) { location.href = 'q3rally-ui://online/address/' + action + '/' + encodeURIComponent(address); }
  function fillSelect(id,kind,value) {
    const select = find(id), values = options[kind] || [], signature = JSON.stringify(values);
    if (select.dataset.options !== signature) { select.replaceChildren(...values.map((title,index) => new Option(title,String(index)))); select.dataset.options = signature; }
    if (select.value !== String(value)) select.value = value;
  }
  function detail() {
    const server = servers.find(item => item.address === selected);
    find('name').textContent = server ? clean(server.name) : 'Choose a server';
    find('address').textContent = server?.address || 'Select a server from the list or connect directly.';
    const details = find('details'); details.replaceChildren();
    if (server) {
      const entries = [['Game mode',server.mode],['Map',server.map || 'Unknown'],['Players',server.clients + ' / ' + server.max],['Human players',server.humans],['Ping',server.ping > 0 ? server.ping + ' ms' : 'No response']];
      entries.forEach(([name,value]) => { const card = node('div');card.append(node('p',name,'label'),node('p',value));details.append(card); });
    }
    const source = catalog.mapShots?.['levelshots/' + server?.map.toLowerCase()];
    const image = find('image'); image.hidden = !source; if (source && image.getAttribute('src') !== source) image.src = source;
    find('connect').disabled = !server;
    find('favorite').disabled = !server || server.favorite;
    find('favorite').textContent = server?.favorite ? 'Saved favorite' : 'Save favorite';
    find('remove').hidden = filters[0] !== 7; find('remove').disabled = !server || !server.favorite;
    find('message').textContent = message;
  }
  function list() {
    const target = find('list'), query = find('search').value.trim().toLowerCase();
    const visible = servers.filter(server => [server.name,server.map,server.address,server.mode].some(value => clean(value).toLowerCase().includes(query)));
    const keys = new Set(visible.map(server => server.address));
    for (const element of [...target.children]) if (!keys.has(element.dataset.address)) element.remove();
    visible.forEach((server,index) => {
      let button = [...target.children].find(element => element.dataset.address === server.address);
      if (!button) {
        button = node('button',undefined,'secondary online-server'); button.dataset.address = server.address;
        button.append(node('span',undefined,'online-server-name'),node('span',undefined,'online-server-players'),node('span',undefined,'online-server-ping'));
        button.addEventListener('click', () => { selected = server.address; list(); detail(); addressAction(5,server.address); });
      }
      button.setAttribute('aria-pressed',String(server.address === selected));
      const name = button.querySelector('.online-server-name'); name.replaceChildren(node('strong',server.name),node('small',(server.map || 'Unknown map') + ' / ' + server.mode + (server.favorite ? ' / Favorite' : '')));
      button.querySelector('.online-server-players').textContent = (filters[5] ? server.humans : server.clients) + ' / ' + server.max;
      button.querySelector('.online-server-ping').textContent = server.ping > 0 ? server.ping + ' ms' : '--';
      if (target.children[index] !== button) target.insertBefore(button,target.children[index] || null);
    });
    if (!visible.length) { const empty = node('p',scanning ? 'Looking for servers...' : servers.length ? 'No servers match your search.' : 'No servers found. Try another source, refresh, or use Direct connect.'); empty.className='online-empty-state'; target.append(empty); }
    find('count').textContent = visible.length + ' / ' + servers.length + ' servers';
  }
  function render(status) {
    fillSelect('source',4,filters[0]);fillSelect('mode',5,filters[1]);fillSelect('sort',6,filters[2]);
    ['full','empty','humans'].forEach((id,index) => { find(id).checked = !!filters[index + 3]; });
    find('refresh').disabled = scanning;find('stop').disabled = !scanning;
    find('status').textContent = scanning ? clean(status || 'Searching for servers...') : clean(status || (servers.length + ' servers found'));
    list();detail();
  }
  find('source').addEventListener('change',event => { selected='';send(1,Number(event.target.value)); });
  find('mode').addEventListener('change',event => send(2,Number(event.target.value)));
  find('sort').addEventListener('change',event => send(3,Number(event.target.value)));
  ['full','empty','humans'].forEach((id,index) => find(id).addEventListener('change',event => send(4,index,event.target.checked ? 1 : 0)));
  find('search').addEventListener('input',list);
  find('back').addEventListener('click', () => send(0));find('refresh').addEventListener('click', () => send(10));find('stop').addEventListener('click', () => send(11));find('host').addEventListener('click', () => send(12));
  find('connect').addEventListener('click', () => { if(selected) addressAction(6,selected); });
  find('favorite').addEventListener('click', () => { if(selected) addressAction(7,selected); });
  find('remove').addEventListener('click', () => { pendingRemoval=selected;find('remove-address').textContent=selected;find('remove-dialog').showModal(); });
  find('remove-confirm').addEventListener('click', () => { find('remove-dialog').close(); if(pendingRemoval) addressAction(8,pendingRemoval); pendingRemoval=''; });
  find('remove-cancel').addEventListener('click', () => { find('remove-dialog').close();pendingRemoval=''; });
  find('direct').addEventListener('click', () => { find('direct-error').textContent='';find('direct-connect').disabled=false;find('direct-dialog').showModal();find('direct-address').focus(); });
  find('direct-close').addEventListener('click', () => find('direct-dialog').close());
  find('direct-form').addEventListener('submit',event => { event.preventDefault(); const address=find('direct-address').value.trim();if(!address){find('direct-error').textContent='Enter a server address.';return;}find('direct-connect').disabled=true;find('direct-error').textContent='Connecting...';addressAction(9,address); });
  find('image').addEventListener('error', () => {find('image').hidden=true;});
  window.q3rallyOnline = {
    begin(source,mode,sort,full,empty,humans,refreshing) { filters=[source,mode,sort,full,empty,humans];scanning=!!refreshing;servers=[];options={}; },
    option(kind,index,title) { (options[kind]??=[])[index]=title; },
    server(name,address,map,mode,clients,max,ping,humans,favorite) { servers.push({name,address,map,mode,clients,max,ping,humans,favorite:!!favorite}); },
    end(status,feedback,address) { message=feedback;selected=address;render(status);if(find('direct-dialog').open && feedback){find('direct-error').textContent=feedback;find('direct-connect').disabled=false;} },
    show(screen) { root.hidden=screen!==11;if(screen!==11){find('direct-dialog').close();find('remove-dialog').close();} }
  };
  fetch('./catalog.json').then(response=>response.json()).then(value=>{catalog=value;if(!root.hidden) detail();}).catch(()=>{});
})();
