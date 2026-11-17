// Measure the WebUI's end credits (screen 3) in Chrome: the reference the
// native credits (q3_ui/ui_rally_creditslayout.c) are tested against. The
// roll is paused at fixed times, so the moving lines can be compared too.
//
//   node tools/ui_webstyle/measure_webui_credits.cjs <staged garage> <fixture.json> [screenshot dir]
//
// The credit lines come from engine/code/q3_ui/ui_credits.c, as the client
// publishes them (Web_PublishItems). See measure_webui_loading.cjs for the
// staged garage and the dependencies.
const {chromium}=require('playwright');const fs=require('fs'),path=require('path'),http=require('http');
const [root,output,shots]=process.argv.slice(2);
const repo=path.resolve(__dirname,'../..');
const version=fs.readFileSync(path.join(repo,'engine/Makefile'),'utf8').match(/^VERSION=(\S+)/m)[1];
const source=fs.readFileSync(path.join(repo,'engine/code/q3_ui/ui_credits.c'),'utf8');
const table=source.slice(source.indexOf('static cr_line_t credits[]'),source.indexOf('{ NULL }'));
const items=[...table.matchAll(/\{\s*((?:PRODUCT_VERSION\s*)?"[^"]*")\s*,\s*([^,]+),\s*&(\w+)/g)].map(m=>{
 const text=m[1].replace(/PRODUCT_VERSION\s*"/,'"'+version).slice(1,-1);
 return [text,/UI_BIGFONT/.test(m[2])?2:m[3]==='color_header'?1:0];});
const types={'.html':'text/html','.js':'text/javascript','.css':'text/css','.json':'application/json','.png':'image/png','.webp':'image/webp','.glb':'model/gltf-binary','.ttf':'font/ttf'};
const server=http.createServer((req,res)=>{const f=path.resolve(root,'.'+decodeURIComponent(new URL(req.url,'http://x').pathname));
 if(!f.startsWith(path.resolve(root))||!fs.existsSync(f)||!fs.statSync(f).isFile()){res.writeHead(404);res.end();return;}
 res.setHeader('Content-Type',types[path.extname(f)]||'application/octet-stream');fs.createReadStream(f).pipe(res);});
const sizes=[[1920,1080],[1280,720],[1366,768],[1600,900],[2560,1440],[1024,768],[800,600],[1280,1024],[3840,2160],[1680,1050]];
const times=[0,6,20,45];	// seconds into the roll
const elements={h1:'#credits-screen h1',exit:'#credits-exit',window:'#credits-window',roll:'#credits-roll',
 hint:'#credits-screen > p.label'};
(async()=>{await new Promise(r=>server.listen(0,'127.0.0.1',r));
 const browser=await chromium.launch({headless:true,...(process.env.WEBUI_BROWSER?{executablePath:process.env.WEBUI_BROWSER}:{}),args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:1920,height:1080}});
 await page.goto('http://127.0.0.1:'+server.address().port+'/index.html');
 await page.waitForFunction(()=>!document.querySelector('#apply').disabled);
 await page.evaluate(()=>document.fonts.ready);
 const result={items};
 for(const [w,h] of sizes){
  await page.setViewportSize({width:w,height:h});
  for(const t of times){
   await page.evaluate(([items,t])=>new Promise(done=>{
    q3rallyFrontend.state(3,0,0,0);q3rallyArchive.creditsBegin();
    items.forEach(([text,kind],i)=>q3rallyArchive.item(3,i,text,0,kind));
    q3rallyArchive.creditsEnd();
    requestAnimationFrame(()=>requestAnimationFrame(()=>{
     const a=document.querySelector('#credits-roll').getAnimations()[0];a.pause();a.currentTime=t*1000;done();}));
   }),[items,t]);
   await page.waitForTimeout(100);
   result[`${w}x${h}_${t}`]=await page.evaluate(elements=>{
    const round=v=>Math.round(v*1000)/1000, box=e=>{const r=e.getBoundingClientRect();return [r.left,r.top,r.width,r.height].map(round);};
    const els={};
    for(const [key,selector] of Object.entries(elements)){const e=document.querySelector(selector);if(e&&e.getClientRects().length)els[key]=box(e);}
    els.lines=[...document.querySelectorAll('#credits-roll > *')].map(box);
    els.rollHeight=document.querySelector('#credits-roll').scrollHeight;
    const texts=[];
    const walker=document.createTreeWalker(document.querySelector('#credits-screen'),NodeFilter.SHOW_TEXT);let node;
    while((node=walker.nextNode())){
     const e=node.parentElement;if(!node.textContent.trim()||!e.getClientRects().length)continue;
     const style=getComputedStyle(e),range=document.createRange();range.selectNodeContents(node);
     texts.push({text:node.textContent,family:style.fontFamily.split(',')[0].replace(/"/g,'').replace(/'/g,''),size:parseFloat(style.fontSize),
      weight:+style.fontWeight,italic:style.fontStyle==='italic',spacing:parseFloat(style.letterSpacing)||0,
      uppercase:style.textTransform==='uppercase',color:style.color,lines:[...range.getClientRects()].map(r=>[r.left,r.top,r.width,r.height].map(round))});
    }
    return {els,texts,height:document.documentElement.getBoundingClientRect().height};
   },elements);
   if(shots&&(w===1920||w===800))await page.screenshot({path:path.join(shots,`webcredits_${w}x${h}_${t}.png`)});
  }
 }
 fs.writeFileSync(output,JSON.stringify(result)+'\n');
 await browser.close();server.close();})();
