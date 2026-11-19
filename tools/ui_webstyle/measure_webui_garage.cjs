// Measure the WebUI's garage (#garage, screen 7) in Chrome: the reference
// the native garage (q3_ui/ui_rally_garage.c) is tested against.
//
//   node tools/ui_webstyle/measure_webui_garage.cjs <staged garage> <fixture.json> [screenshot dir]
//
// The staged garage is engine/webui/build_showroom.py's output: its
// catalog.json is the game's own vehicles, paints, wheels, drivers and
// plates. Every visible element of #garage is recorded under a path
// (tag#id.class:index), with the carousel cards' texts.
const {chromium}=require('playwright');const fs=require('fs'),path=require('path'),http=require('http');
const [root,output,shots]=process.argv.slice(2);
const types={'.html':'text/html','.js':'text/javascript','.css':'text/css','.json':'application/json','.png':'image/png','.webp':'image/webp','.glb':'model/gltf-binary','.ttf':'font/ttf'};
const server=http.createServer((req,res)=>{const f=path.resolve(root,'.'+decodeURIComponent(new URL(req.url,'http://x').pathname));
 if(!f.startsWith(path.resolve(root))||!fs.existsSync(f)||!fs.statSync(f).isFile()){res.writeHead(404);res.end();return;}
 res.setHeader('Content-Type',types[path.extname(f)]||'application/octet-stream');fs.createReadStream(f).pipe(res);});
const sizes=[[1920,1080],[1280,720],[1366,768],[1600,900],[2560,1440],[1024,768],[800,600],[1280,1024],[3840,2160],[1680,1050]];
/* [name, vehicle, paint, wheels, unlocked rank, driver, plate, favourites] */
const states=[
 ['default','alpine','blue','svt_cobra',3,'classic','usa_california',[['alpine','blue','svt_cobra','classic','usa_california'],['viper','red','amg_163','camo','default']]],
 ['locked','alpine','chrome','svt_cobra',3,'classic','usa_california',[]],
 ['rookie','bountyhunter','','svt_cobra',0,'classic','usa_california',[]],
];
(async()=>{await new Promise(r=>server.listen(0,'127.0.0.1',r));
 const browser=await chromium.launch({headless:true,...(process.env.WEBUI_BROWSER?{executablePath:process.env.WEBUI_BROWSER}:{}),args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:1920,height:1080}});
 await page.goto('http://127.0.0.1:'+server.address().port+'/index.html');
 await page.waitForFunction(()=>!document.querySelector('#apply').disabled||document.querySelector('#status').textContent.includes('unlocks'));
 await page.evaluate(()=>document.fonts.ready);
 await page.waitForTimeout(500);
 const result={};
 for(const [w,h] of sizes){
  await page.setViewportSize({width:w,height:h});
  for(const [name,id,paint,wheels,rank,driver,plate,favorites] of states){
   await page.evaluate(([id,paint,wheels,rank,driver,plate,favorites])=>{
    q3rallyFrontend.state(0,0,0,0);q3rallyFrontend.state(7,0,0,0);
    q3rallyGarage.select(id,paint,wheels,rank,driver,plate);
    for(let slot=0;slot<4;slot++)q3rallyGarage.favorite(slot,...(favorites[slot]??['','','','','']));
    document.querySelector('#web-profile-status').textContent='';
    document.activeElement?.blur?.();scrollTo(0,0);
   },[id,paint,wheels,rank,driver,plate,favorites]);
   await page.waitForFunction(()=>[...document.querySelectorAll('#garage img')].every(i=>i.hidden||i.complete));
   await page.waitForTimeout(shots&&(w===1920||w===1280||w===800)?800:150);
   result[`${w}x${h}_${name}`]=await page.evaluate(()=>{
    const round=v=>Math.round(v*1000)/1000, box=e=>{const r=e.getBoundingClientRect();return [r.left,r.top,r.width,r.height].map(round);};
    const els={},texts={};
    const walk=(e,prefix)=>{[...e.children].forEach((c,i)=>{
     if(!c.getClientRects().length||c.tagName==='OPTION'||c.tagName==='SELECT')return;
     const key=prefix+'/'+c.tagName.toLowerCase()+(c.id?'#'+c.id:'')+(c.className&&typeof c.className==='string'?'.'+c.className.trim().split(/\s+/).join('.'):'')+':'+i;
     els[key]=box(c);
     if(!c.children.length&&c.textContent)texts[key]=c.textContent;
     walk(c,key);});};
    walk(document.querySelector('#garage'),'');
    return {els,texts,disabled:[...document.querySelectorAll('#garage button')].filter(b=>b.disabled&&b.getClientRects().length).map(b=>b.id||b.textContent),
     height:document.documentElement.getBoundingClientRect().height};
   });
   if(shots&&(w===1920||w===1280||w===800))await page.screenshot({path:path.join(shots,`webgarage_${w}x${h}_${name}.png`)});
  }
 }
 fs.writeFileSync(output,JSON.stringify(result)+'\n');
 await browser.close();server.close();})();
