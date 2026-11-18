// Measure the WebUI's map loading screen (screen 14) in Chrome: the reference
// the native one (q3_ui/ui_rally_maploadlayout.c) is tested against.
//
//   node tools/ui_webstyle/measure_webui_maploading.cjs <staged garage> <fixture.json> [screenshot dir]
//
// The states are what the client publishes (Web_PublishMapLoading). See
// measure_webui_loading.cjs for the staged garage and the dependencies.
const {chromium}=require('playwright');const fs=require('fs'),path=require('path'),http=require('http');
const [root,output,shots]=process.argv.slice(2);
const types={'.html':'text/html','.js':'text/javascript','.css':'text/css','.json':'application/json','.png':'image/png','.webp':'image/webp','.glb':'model/gltf-binary','.ttf':'font/ttf'};
const server=http.createServer((req,res)=>{const f=path.resolve(root,'.'+decodeURIComponent(new URL(req.url,'http://x').pathname));
 if(!f.startsWith(path.resolve(root))||!fs.existsSync(f)||!fs.statSync(f).isFile()){res.writeHead(404);res.end();return;}
 res.setHeader('Content-Type',types[path.extname(f)]||'application/octet-stream');fs.createReadStream(f).pipe(res);});
const sizes=[[1920,1080],[1280,720],[1366,768],[1600,900],[2560,1440],[1024,768],[800,600],[1280,1024],[3840,2160],[1680,1050]];
// name, then state(map, title, server, detail, download, count, total, attempt)
const states=[['connecting',['','','race.example.net:27960','Awaiting challenge','',0,0,3]],
 ['loading',['q3r_canyonrush','Canyon ^1Rush','localhost','Loading vehicles','',0,0,0]],
 ['download',['q3r_battlecanyon','','race.example.net:27960','Receiving game state','maps/q3r_newtrack.pk3',3407872,9437184,0]]];
const elements={h1:'#map-loading-screen h1',preview:'.map-loading-preview',status:'.map-loading-status',
 image:'#map-loading-image',name:'#map-loading-name',server:'#map-loading-server',statusLabel:'.map-loading-status .label',
 detail:'#map-loading-detail',progress:'#map-loading-progress',transfer:'#map-loading-transfer',
 note:'.map-loading-status p:not([id]):not(.label)',cancel:'#map-loading-cancel'};
(async()=>{await new Promise(r=>server.listen(0,'127.0.0.1',r));
 const browser=await chromium.launch({headless:true,...(process.env.WEBUI_BROWSER?{executablePath:process.env.WEBUI_BROWSER}:{}),args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:1920,height:1080}});
 await page.goto('http://127.0.0.1:'+server.address().port+'/index.html');
 await page.waitForFunction(()=>!document.querySelector('#apply').disabled);
 await page.evaluate(()=>document.fonts.ready);
 const result={};
 for(const [w,h] of sizes){
  await page.setViewportSize({width:w,height:h});
  for(const [name,state] of states){
   await page.evaluate(state=>{q3rallyFrontend.state(0,0,0,0);q3rallyFrontend.state(14,0,0,0);q3rallyMapLoading.state(...state);},state);
   await page.waitForTimeout(800);
   await page.evaluate(()=>{document.activeElement.blur();scrollTo(0,0);});
   result[`${w}x${h}_${name}`]=await page.evaluate(elements=>{
    const round=v=>Math.round(v*1000)/1000, box=e=>{const r=e.getBoundingClientRect();return [r.left,r.top,r.width,r.height].map(round);};
    const els={};
    for(const [key,selector] of Object.entries(elements)){const e=document.querySelector(selector);if(e&&e.getClientRects().length)els[key]=box(e);}
    els.progressValue=document.querySelector('#map-loading-progress').hasAttribute('value')?document.querySelector('#map-loading-progress').value:-1;
    const texts=[];
    const walker=document.createTreeWalker(document.querySelector('#map-loading-screen'),NodeFilter.SHOW_TEXT);let node;
    while((node=walker.nextNode())){
     const e=node.parentElement;if(!node.textContent.trim()||!e.getClientRects().length)continue;
     const style=getComputedStyle(e),range=document.createRange();range.selectNodeContents(node);
     texts.push({text:node.textContent,family:style.fontFamily.split(',')[0].replace(/"/g,'').replace(/'/g,''),size:parseFloat(style.fontSize),
      weight:+style.fontWeight,italic:style.fontStyle==='italic',spacing:parseFloat(style.letterSpacing)||0,
      uppercase:style.textTransform==='uppercase',color:style.color,lines:[...range.getClientRects()].map(r=>[r.left,r.top,r.width,r.height].map(round))});
    }
    return {els,texts,height:document.documentElement.getBoundingClientRect().height};
   },elements);
   if(shots&&(w===1920||w===800))await page.screenshot({path:path.join(shots,`webmaploading_${w}x${h}_${name}.png`)});
  }
 }
 fs.writeFileSync(output,JSON.stringify(result)+'\n');
 await browser.close();server.close();})();
