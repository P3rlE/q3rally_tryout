// Measure the WebUI's HUD options (#hud-screen, screen 13, the panel at the
// right of the race) in Chrome: the reference the native panel
// (cgame/cg_hud_panel.c) is tested against.
//
//   node tools/ui_webstyle/measure_webui_hud.cjs <staged garage> <fixture.json> [screenshot dir]
//
// tests/fixtures/hud_model.json holds the toggles of cgame's hudToggleTable
// (cg_hud_core.c) as a circuit race shows them. Every visible element is recorded
// under a path (tag#id.class:index from the panel down).
const {chromium}=require('playwright');const fs=require('fs'),path=require('path'),http=require('http');
const [root,output,shots]=process.argv.slice(2);
const repo=path.resolve(__dirname,'../..');
const model=JSON.parse(fs.readFileSync(path.join(repo,'tests/fixtures/hud_model.json'),'utf8'));
const types={'.html':'text/html','.js':'text/javascript','.css':'text/css','.json':'application/json','.png':'image/png','.webp':'image/webp','.glb':'model/gltf-binary','.ttf':'font/ttf'};
const server=http.createServer((req,res)=>{const f=path.resolve(root,'.'+decodeURIComponent(new URL(req.url,'http://x').pathname));
 if(!f.startsWith(path.resolve(root))||!fs.existsSync(f)||!fs.statSync(f).isFile()){res.writeHead(404);res.end();return;}
 res.setHeader('Content-Type',types[path.extname(f)]||'application/octet-stream');fs.createReadStream(f).pipe(res);});
const sizes=[[1920,1080],[1280,720],[1366,768],[1600,900],[2560,1440],[1024,768],[800,600],[1280,1024],[3840,2160],[1680,1050]];
(async()=>{await new Promise(r=>server.listen(0,'127.0.0.1',r));
 const browser=await chromium.launch({headless:true,...(process.env.WEBUI_BROWSER?{executablePath:process.env.WEBUI_BROWSER}:{}),args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:1920,height:1080}});
 await page.goto('http://127.0.0.1:'+server.address().port+'/index.html');
 await page.waitForFunction(()=>!document.querySelector('#apply').disabled);
 await page.evaluate(()=>document.fonts.ready);
 await page.waitForTimeout(500);
 const result={};
 for(const [w,h] of sizes){
  await page.setViewportSize({width:w,height:h});
  await page.evaluate(model=>{
   q3rallyFrontend.state(0,0,0,0);q3rallyFrontend.state(13,0,0,0);
   q3rallyHUD.begin();
   model.items.forEach(([index,label,value,disabled,choices])=>{q3rallyHUD.item(index,label,value,disabled);choices.forEach(c=>q3rallyHUD.choice(index,c));});
   q3rallyHUD.end(model.size,model.zoom);
   document.activeElement?.blur?.();document.querySelector('#hud-screen').scrollTop=0;
  },model);
  await page.waitForTimeout(shots?800:200);
  result[`${w}x${h}`]=await page.evaluate(()=>{
   const round=v=>Math.round(v*1000)/1000, box=e=>{const r=e.getBoundingClientRect();return [r.left,r.top,r.width,r.height].map(round);};
   const els={};
   const walk=(e,prefix)=>{[...e.children].forEach((c,i)=>{
    if(!c.getClientRects().length||c.tagName==='OPTION')return;
    const key=prefix+'/'+c.tagName.toLowerCase()+(c.id?'#'+c.id:'')+(c.className&&typeof c.className==='string'?'.'+c.className.trim().split(/\s+/).join('.'):'')+':'+i;
    els[key]=box(c);walk(c,key);});};
   const screen=document.querySelector('#hud-screen');els['panel']=box(screen);walk(screen,'');
   return {els,scrollHeight:screen.scrollHeight,clientHeight:screen.clientHeight};
  });
  if(shots)await page.screenshot({path:path.join(shots,`webhud_${w}x${h}.png`)});
 }
 fs.writeFileSync(output,JSON.stringify(result)+'\n');
 await browser.close();server.close();})();
