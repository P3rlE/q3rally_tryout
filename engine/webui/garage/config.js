'use strict';
(() => {
  const root=document.querySelector('#config-screen'),find=id=>document.querySelector('#config-'+id);
  let page=0,title='',description='',confirmation=false,items=[],capture,focusHint,previousPage='',searchTimer;
  const clean=text=>String(text||'').replace(/\^[0-9]/g,'');
  const node=(tag,text,className)=>{const e=document.createElement(tag);if(text!==undefined)e.textContent=clean(text);if(className)e.className=className;return e;};
  const send=(action,index=0,value=0)=>{location.href='q3rally-ui://config/'+action+'/'+index+'/'+value;};
  function finishCapture(key) { if(capture===undefined)return;const index=capture;capture=undefined;find('capture').close();if(key)location.href='q3rally-ui://config/key/'+index+'/'+encodeURIComponent(key); }
  function startCapture(item) { capture=item.index;padNeutral=false;find('capture-action').textContent=item.label;find('capture').showModal(); }
  const keyNames={Enter:'ENTER',Tab:'TAB',Space:'SPACE',Backspace:'BACKSPACE',Delete:'DEL',Insert:'INS',Home:'HOME',End:'END',PageUp:'PGUP',PageDown:'PGDN',ArrowUp:'UPARROW',ArrowDown:'DOWNARROW',ArrowLeft:'LEFTARROW',ArrowRight:'RIGHTARROW',ShiftLeft:'SHIFT',ShiftRight:'SHIFT',ControlLeft:'CTRL',ControlRight:'CTRL',AltLeft:'ALT',AltRight:'ALT',Pause:'PAUSE',CapsLock:'CAPSLOCK',Numpad0:'KP_INS',Numpad1:'KP_END',Numpad2:'KP_DOWNARROW',Numpad3:'KP_PGDN',Numpad4:'KP_LEFTARROW',Numpad5:'KP_5',Numpad6:'KP_RIGHTARROW',Numpad7:'KP_HOME',Numpad8:'KP_UPARROW',Numpad9:'KP_PGUP',NumpadEnter:'KP_ENTER',NumpadDecimal:'KP_DEL',NumpadAdd:'KP_PLUS',NumpadSubtract:'KP_MINUS',NumpadMultiply:'KP_STAR',NumpadDivide:'KP_SLASH'};
  find('capture').addEventListener('keydown',event=>{
    if(capture===undefined)return;event.preventDefault();event.stopPropagation();if(event.repeat)return;
    if(event.key==='Escape'){finishCapture();return;}
    if(event.code==='Backquote')return;
    const key=keyNames[event.code]||keyNames[event.key]||(/^F\d{1,2}$/.test(event.code)?event.code:/^F\d{1,2}$/.test(event.key)?event.key:event.key.length===1?event.key.toLowerCase():null);if(key)finishCapture(key);
  });
  find('capture').addEventListener('mousedown',event=>{if(capture===undefined||event.target.closest('button'))return;event.preventDefault();const mouse=['MOUSE1','MOUSE3','MOUSE2','MOUSE4','MOUSE5'][event.button];if(mouse)finishCapture(mouse);});
  find('capture').addEventListener('contextmenu',event=>event.preventDefault());
  find('capture').addEventListener('wheel',event=>{if(capture!==undefined){event.preventDefault();finishCapture(event.deltaY<0?'MWHEELUP':'MWHEELDOWN');}},{passive:false});
  find('capture').addEventListener('cancel',()=>{capture=undefined;});find('capture-cancel').addEventListener('click',()=>finishCapture());
  const padNames=['A','B','X','Y','LEFTSHOULDER','RIGHTSHOULDER','LEFTTRIGGER','RIGHTTRIGGER','BACK','START','LEFTSTICK_CLICK','RIGHTSTICK_CLICK','DPAD_UP','DPAD_DOWN','DPAD_LEFT','DPAD_RIGHT','GUIDE'];
  let padTimer, padNeutral=false;
  function pollPad(){
    if(capture===undefined){padTimer=undefined;return;}
    const pad=[...(navigator.getGamepads?.()||[])].find(p=>p?.connected);
    if(pad){
      const index=pad.buttons.findIndex(button=>button.pressed),axis=pad.axes.slice(0,4).findIndex(value=>Math.abs(value)>.75);
      if(index<0&&axis<0)padNeutral=true;
      if(padNeutral&&index>=0&&padNames[index]){finishCapture('PAD0_'+padNames[index]);padTimer=undefined;return;}
      if(padNeutral&&axis>=0){const stick=axis<2?'LEFTSTICK_':'RIGHTSTICK_',direction=axis%2?(pad.axes[axis]<0?'UP':'DOWN'):(pad.axes[axis]<0?'LEFT':'RIGHT');finishCapture('PAD0_'+stick+direction);padTimer=undefined;return;}
    }
    padTimer=requestAnimationFrame(pollPad);
  }
  find('capture').addEventListener('focusin',()=>{if(!padTimer)padTimer=requestAnimationFrame(pollPad);});
  function button(item){const b=node('button',item.label,'secondary');b.disabled=item.disabled;b.addEventListener('click',()=>send(0,item.index));return b;}
  function field(item){
    const wrapper=node('div',undefined,'config-field'),label=node('label',item.label.replace(/:\s*$/,''));label.htmlFor='config-item-'+item.index;wrapper.append(label);
    if(item.type===5){const actions=node('div',undefined,'config-actions'),rebind=node('button',item.text||'Unbound','secondary'),clear=node('button','Clear','secondary');rebind.disabled=clear.disabled=item.disabled;rebind.addEventListener('click',()=>startCapture(item));clear.addEventListener('click',()=>send(5,item.index));actions.append(rebind,clear);wrapper.append(actions);return wrapper;}
    if(item.type===8){const select=node('select');select.add(new Option('Choose an option',''));item.choices.forEach((text,index)=>select.add(new Option(text,String(index))));select.disabled=item.disabled;select.addEventListener('change',()=>{if(select.value!=='')send(7,item.index,Number(select.value));});wrapper.append(select);return wrapper;}
    if(item.type===7){wrapper.append(node('pre',item.choices.join('\n')));return wrapper;}
    let input;
    if(item.type===1||item.type===2){input=node('select');if(item.value<0){const custom=new Option('Custom','-1');custom.disabled=true;input.add(custom);}item.choices.forEach((name,index)=>input.add(new Option(clean(name),String(index))));input.value=item.value;}
    else if(item.type===3){input=node('input');input.type='range';input.min=item.min;input.max=item.max;input.step=item.max-item.min<=100?1:100;input.value=item.value;const output=node('output',String(item.value/100));input.addEventListener('input',()=>{output.textContent=String(Number(input.value)/100);});wrapper.append(output);}
    else{input=node('input');input.type='search';input.value=item.text;input.placeholder='Search controls';}
    input.id='config-item-'+item.index;input.disabled=item.disabled;input.addEventListener('focus',()=>{focusHint=input.id;});
    function change(){if(item.type===4)location.href='q3rally-ui://config/text/'+item.index+'/'+encodeURIComponent(input.value);else send(item.type===3?2:1,item.index,Number(input.value));}
    if(item.type===4)input.addEventListener('input',()=>{clearTimeout(searchTimer);searchTimer=setTimeout(change,250);});else input.addEventListener('change',change);
    wrapper.append(input);return wrapper;
  }
  function render(){
    const pageKey=page+':'+title;if(pageKey!==previousPage){focusHint=undefined;window.scrollTo(0,0);}previousPage=pageKey;
    find('title').textContent=title;find('description').textContent=description;
    root.querySelector('header .label').textContent=page>=10?'Q3RALLY / IN RACE':'Q3RALLY / CONFIGURATION';
    find('back').textContent=page===10?'Resume race':'Back';
    const actions=find('actions'),fields=find('fields');actions.replaceChildren();fields.replaceChildren();
    const controls=items.filter(item=>item.type!==6),buttons=items.filter(item=>item.type===6 && (confirmation || item.label.trim().toLowerCase()!=='back' && !(page===10 && /^resume/i.test(item.label))));
    find('settings').hidden=!controls.length;
    if(confirmation){find('confirm-title').textContent=description||'Confirm change';find('confirm-message').textContent='';find('confirm-actions').replaceChildren(...buttons.map(button));if(!find('confirm').open)find('confirm').showModal();}
    else{find('confirm').close();buttons.forEach(item=>actions.append(button(item)));controls.forEach(item=>fields.append(field(item)));if(focusHint)document.getElementById(focusHint)?.focus({preventScroll:true});}
  }
  root.addEventListener('keydown',event=>{if(event.key==='Escape'&&!event.target.closest('dialog')){event.preventDefault();send(6);}});
  find('back').addEventListener('click',()=>send(6));find('confirm').addEventListener('cancel',event=>{event.preventDefault();send(6);});
  window.q3rallyConfig={
    begin(nextPage,nextTitle,nextDescription,isConfirmation){page=nextPage;title=nextTitle;description=nextDescription;confirmation=!!isConfirmation;items=[];},
    item(index,type,label,text,value,min,max,disabled){items.push({index,type,label,text,value,min,max,disabled:!!disabled,choices:[]});},
    choice(index,choice,label){items.find(item=>item.index===index)?.choices.push(label);},
    end(){render();},
    show(screen){root.hidden=screen!==12;if(screen!==12){finishCapture();find('confirm').close();}}
  };
})();
