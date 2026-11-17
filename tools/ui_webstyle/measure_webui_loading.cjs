// Measure the WebUI's loading screen (screen 1) in Chrome: the reference the
// native screen (q3_ui/ui_rally_gfxlayout.c) is tested against.
//
//   node tools/ui_webstyle/measure_webui_loading.cjs <staged garage> <fixture.json> [screenshot dir]
//
// <staged garage> is the bundle build_showroom.py stages (catalog, showroom);
// run from engine/webui so the playwright dependency resolves. Writes the box
// of every element and the width of every text run for ten viewports and the
// four states the screen has.
const {chromium}=require('playwright');const fs=require('fs'),path=require('path'),http=require('http');
const [root,output,shots]=process.argv.slice(2);
const types={'.html':'text/html','.js':'text/javascript','.css':'text/css','.json':'application/json','.png':'image/png','.webp':'image/webp','.glb':'model/gltf-binary','.ttf':'font/ttf'};
const server=http.createServer((req,res)=>{const f=path.resolve(root,'.'+decodeURIComponent(new URL(req.url,'http://x').pathname));
 if(!f.startsWith(path.resolve(root))||!fs.existsSync(f)||!fs.statSync(f).isFile()){res.writeHead(404);res.end();return;}
 res.setHeader('Content-Type',types[path.extname(f)]||'application/octet-stream');fs.createReadStream(f).pipe(res);});
const sizes=[[1920,1080],[1280,720],[1366,768],[1600,900],[2560,1440],[1024,768],[800,600],[1280,1024],[3840,2160],[1680,1050]];
// name, percent, stage, update prompt (q3rallyFrontend.state(1, ...))
const states=[['s2',33,2,0],['s0',0,0,0],['s8',100,8,0],['prompt',100,8,1]];
const elements={left:'#loading-panel',right:'.frontend-showroom',h1:'#frontend h1',status:'#frontend-status',
 title:'#loading-title',percent:'#loading-percent',progress:'#loading-progress',vehicle:'#active-vehicle-name',
 car:'#active-vehicle-3d',statusText:'#active-vehicle-status',prompt:'#update-prompt p',
 update:'#update-prompt button[data-action=update]',skip:'#update-prompt button[data-action=skip]'};
(async()=>{await new Promise(r=>server.listen(0,'127.0.0.1',r));
 const browser=await chromium.launch({headless:true,...(process.env.WEBUI_BROWSER?{executablePath:process.env.WEBUI_BROWSER}:{}),args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:1920,height:1080}});
 await page.goto('http://127.0.0.1:'+server.address().port+'/index.html');
 await page.waitForFunction(()=>!document.querySelector('#apply').disabled);
 await page.evaluate(()=>document.fonts.ready);
 const result={};
 for(const [w,h] of sizes){
  await page.setViewportSize({width:w,height:h});
  for(const [name,percent,stage,prompt] of states){
   await page.evaluate(([p,s,u])=>{q3rallyFrontend.vehicle('alpine','red','svt_cobra');q3rallyFrontend.state(1,p,s,u);},[percent,stage,prompt]);
   await page.waitForTimeout(shots&&name==='s2'?2500:200);
   result[`${w}x${h}_${name}`]=await page.evaluate(elements=>{
    const round=v=>Math.round(v*1000)/1000, box=e=>{const r=e.getBoundingClientRect();return [r.left,r.top,r.width,r.height].map(round);};
    const els={};
    for(const [key,selector] of Object.entries(elements)){const e=document.querySelector(selector);if(e&&e.getClientRects().length)els[key]=box(e);}
    els.stages=[...document.querySelectorAll('#loading-stages li')].map(box);
    const texts=[];const walker=document.createTreeWalker(document.querySelector('#frontend'),NodeFilter.SHOW_TEXT);let node;
    while((node=walker.nextNode())){
     const e=node.parentElement;if(!node.textContent.trim()||!e.getClientRects().length)continue;
     const style=getComputedStyle(e),range=document.createRange();range.selectNodeContents(node);
     texts.push({text:node.textContent,family:style.fontFamily.split(',')[0].replace(/"/g,''),size:parseFloat(style.fontSize),
      weight:+style.fontWeight,italic:style.fontStyle==='italic',spacing:parseFloat(style.letterSpacing)||0,
      uppercase:style.textTransform==='uppercase',lines:[...range.getClientRects()].map(r=>[r.left,r.top,r.width,r.height].map(round))});
    }
    return {els,texts,height:document.documentElement.getBoundingClientRect().height};
   },elements);
   if(shots&&(name==='s2'||name==='prompt'))await page.screenshot({path:path.join(shots,`web_${w}x${h}_${name}.png`)});
  }
 }
 fs.writeFileSync(output,JSON.stringify(result)+'\n');
 await browser.close();server.close();})();
