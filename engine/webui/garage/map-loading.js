'use strict';
(()=>{
 const root=document.querySelector('#map-loading-screen'),find=id=>document.querySelector('#map-loading-'+id);
 const clean=value=>String(value||'').replace(/\^[0-9]/g,'');let currentMap='',revision=0;
 find('cancel').addEventListener('click',()=>{location.href='q3rally-ui://loading/cancel';});
 window.q3rallyMapLoading={
  show(screen){root.hidden=screen!==14;},
  state(map,title,server,detail,download,count,total,attempt){
   find('name').textContent=clean(title||map||'Preparing your race');find('server').textContent=server==='localhost'?'Local race':clean(server);
   find('detail').textContent=download?'Downloading '+clean(download):clean(detail)||'Preparing track resources';
   const progress=find('progress');if(download&&total>0)progress.value=Math.max(0,Math.min(100,count*100/total));else progress.removeAttribute('value');
   find('transfer').textContent=download?(total>0?Math.floor(progress.value)+'% \u00b7 '+(count/1048576).toFixed(1)+' / '+(total/1048576).toFixed(1)+' MB':(count/1048576).toFixed(1)+' MB received'):attempt>0&&/Awaiting/.test(detail)?'Connection attempt '+attempt:'';
   if(currentMap!==map){currentMap=map;const request=++revision;const image=find('image');image.hidden=true;image.removeAttribute('src');if(map)fetch('./catalog.json').then(r=>r.json()).then(c=>{if(request!==revision)return;const source=c.mapShots?.['levelshots/'+map];if(source){image.onload=()=>{if(request===revision)image.hidden=false;};image.onerror=()=>{image.hidden=true;};image.src=source;}}).catch(()=>{});}
  }
 };
})();
