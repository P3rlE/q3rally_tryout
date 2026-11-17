// Measure the WebUI's main menu (screen 2) and its quit dialog in Chrome: the
// reference the native main menu (q3_ui/ui_rally_mainlayout.c) is tested
// against.
//
//   node tools/ui_webstyle/measure_webui_main.cjs <staged garage> <fixture.json> [screenshot dir]
//
// See measure_webui_loading.cjs for the staged garage and the dependencies.
const {chromium}=require('playwright');const fs=require('fs'),path=require('path'),http=require('http');
const [root,output,shots]=process.argv.slice(2);
const types={'.html':'text/html','.js':'text/javascript','.css':'text/css','.json':'application/json','.png':'image/png','.webp':'image/webp','.glb':'model/gltf-binary','.ttf':'font/ttf'};
const server=http.createServer((req,res)=>{const f=path.resolve(root,'.'+decodeURIComponent(new URL(req.url,'http://x').pathname));
 if(!f.startsWith(path.resolve(root))||!fs.existsSync(f)||!fs.statSync(f).isFile()){res.writeHead(404);res.end();return;}
 res.setHeader('Content-Type',types[path.extname(f)]||'application/octet-stream');fs.createReadStream(f).pipe(res);});
const sizes=[[1920,1080],[1280,720],[1366,768],[1600,900],[2560,1440],[1024,768],[800,600],[1280,1024],[3840,2160],[1680,1050]];
// name, profile (name, rank, points) or null, quit dialog open
const states=[['main',['Per',3,420],false],['fresh',['',0,0],false],['quit',['Per',3,420],true]];
const elements={h1:'#frontend h1',status:'#frontend-status',nav:'#main-navigation',right:'.frontend-showroom',
 vehicle:'#active-vehicle-name',car:'#active-vehicle-3d',statusText:'#active-vehicle-status',garage:'#open-garage',
 summary:'#profile-summary',dialog:'#quit-confirm',dialogLabel:'#quit-confirm .label',dialogTitle:'#quit-confirm h2',
 dialogText:'#quit-confirm h2 + p',accept:'#quit-accept',cancel:'#quit-cancel'};
(async()=>{await new Promise(r=>server.listen(0,'127.0.0.1',r));
 const browser=await chromium.launch({headless:true,...(process.env.WEBUI_BROWSER?{executablePath:process.env.WEBUI_BROWSER}:{}),args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:1920,height:1080}});
 await page.goto('http://127.0.0.1:'+server.address().port+'/index.html');
 await page.waitForFunction(()=>!document.querySelector('#apply').disabled);
 await page.evaluate(()=>document.fonts.ready);
 const result={};
 for(const [w,h] of sizes){
  await page.setViewportSize({width:w,height:h});
  for(const [name,profile,quit] of states){
   await page.evaluate(([profile,quit])=>{
    const dialog=document.querySelector('#quit-confirm'); if(dialog.open)dialog.close();
    q3rallyFrontend.vehicle('alpine','red','svt_cobra');q3rallyFrontend.state(2,0,0,0);
    q3rallyFrontend.profile(...profile);
    if(quit){dialog.showModal();document.activeElement.blur();}
   },[profile,quit]);
   await page.waitForTimeout(shots&&name!=='fresh'?2500:300);
   result[`${w}x${h}_${name}`]=await page.evaluate(elements=>{
    const round=v=>Math.round(v*1000)/1000, box=e=>{const r=e.getBoundingClientRect();return [r.left,r.top,r.width,r.height].map(round);};
    const els={};
    for(const [key,selector] of Object.entries(elements)){const e=document.querySelector(selector);if(e&&e.getClientRects().length)els[key]=box(e);}
    els.buttons=[...document.querySelectorAll('#main-navigation button')].map(box);
    const texts=[];
    for(const scope of ['#frontend','#quit-confirm']){
     const walker=document.createTreeWalker(document.querySelector(scope),NodeFilter.SHOW_TEXT);let node;
     while((node=walker.nextNode())){
      const e=node.parentElement;if(!node.textContent.trim()||!e.getClientRects().length)continue;
      const style=getComputedStyle(e),range=document.createRange();range.selectNodeContents(node);
      texts.push({text:node.textContent,family:style.fontFamily.split(',')[0].replace(/"/g,'').replace(/'/g,''),size:parseFloat(style.fontSize),
       weight:+style.fontWeight,italic:style.fontStyle==='italic',spacing:parseFloat(style.letterSpacing)||0,
       uppercase:style.textTransform==='uppercase',color:style.color,lines:[...range.getClientRects()].map(r=>[r.left,r.top,r.width,r.height].map(round))});
     }
    }
    return {els,texts,height:document.documentElement.getBoundingClientRect().height};
   },elements);
   if(shots&&w===1920||shots&&w===800)await page.screenshot({path:path.join(shots,`webmain_${w}x${h}_${name}.png`)});
  }
 }
 fs.writeFileSync(output,JSON.stringify(result)+'\n');
 await browser.close();server.close();})();
