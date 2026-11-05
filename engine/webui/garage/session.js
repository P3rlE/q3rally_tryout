'use strict';
(() => {
  const root=document.querySelector('#hud-screen'),find=id=>document.querySelector('#hud-'+id);
  let items=[],visible=false,scheduled=false;
  const close=()=>{location.href='q3rally-ui://hud/close';};
  const consoleOpen=()=>{location.href='q3rally-ui://session/console';};
  function viewport(){
    scheduled=false;if(!visible)return;const r=root.getBoundingClientRect();
    const x=Math.max(0,Math.round(r.x)),y=Math.max(0,Math.round(r.y)),w=Math.min(innerWidth-x,Math.round(r.width)),h=Math.min(innerHeight-y,Math.round(r.height));
    location.href='q3rally-ui://hud/viewport/'+[x,y,w,h,innerWidth,innerHeight].join('/');
  }
  function measure(){if(!scheduled){scheduled=true;requestAnimationFrame(viewport);}}
  const send=(action,index,value)=>{location.href='q3rally-ui://hud/'+action+'/'+index+'/'+value;};
  find('close').addEventListener('click',close);find('console').addEventListener('click',consoleOpen);
  document.addEventListener('keydown',event=>{
    if(event.repeat)return;
    if(event.code==='Backquote'||(event.key==='Escape'&&event.shiftKey)){event.preventDefault();event.stopImmediatePropagation();consoleOpen();}
    else if(visible&&(event.key==='Escape'||(event.key.toLowerCase()==='h'&&event.shiftKey))){event.preventDefault();close();}
  },true);
  for(const [id,action] of [['size',1],['zoom',2]]){
    find(id).addEventListener('input',()=>{find(id+'-value').textContent=id==='size'?(Number(find(id).value)/100).toFixed(2):find(id).value;});
    find(id).addEventListener('change',()=>send(action,0,Number(find(id).value)));
  }
  window.q3rallyHUD={
    begin(){items=[];},
    item(index,label,value,disabled){items.push({index,label,value,disabled:!!disabled,choices:[]});},
    choice(index,label){items.find(item=>item.index===index)?.choices.push(label);},
    end(size,zoom){
      const focus=document.activeElement?.id;find('fields').replaceChildren(...items.map(item=>{
        const label=document.createElement('label');label.textContent=item.label;
        const select=document.createElement('select');select.id='hud-item-'+item.index;select.disabled=item.disabled;item.choices.forEach((text,index)=>select.add(new Option(text,String(index))));select.value=item.value;
        select.addEventListener('change',()=>send(0,item.index,Number(select.value)));label.append(select);return label;
      }));
      find('size').value=size;find('size-value').textContent=(size/100).toFixed(2);find('zoom').value=zoom;find('zoom-value').textContent=zoom;
      if(focus?.startsWith('hud-item-'))document.getElementById(focus)?.focus({preventScroll:true});measure();
    },
    show(screen){visible=screen===13;root.hidden=!visible;if(visible)measure();}
  };
  new ResizeObserver(measure).observe(root);window.addEventListener('resize',measure);
})();
