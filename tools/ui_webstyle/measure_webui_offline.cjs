// Measure step 1 of the WebUI's offline race setup (screen 9) in Chrome: the
// reference the native one (q3_ui/ui_rally_offlinelayout.c) is tested against.
//
//   node tools/ui_webstyle/measure_webui_offline.cjs <staged garage> <fixture.json> [screenshot dir]
//
// The data is the game's own: tests/fixtures/offline_model.json, as the UI
// prints it with ui_offlineDump 1. See measure_webui_loading.cjs.
const {chromium}=require('playwright');const fs=require('fs'),path=require('path'),http=require('http');
const [root,output,shots]=process.argv.slice(2);
const repo=path.resolve(__dirname,'../..');
const model=JSON.parse(fs.readFileSync(path.join(repo,'tests/fixtures/offline_model.json'),'utf8'));
const types={'.html':'text/html','.js':'text/javascript','.css':'text/css','.json':'application/json','.png':'image/png','.webp':'image/webp','.glb':'model/gltf-binary','.ttf':'font/ttf'};
const server=http.createServer((req,res)=>{const f=path.resolve(root,'.'+decodeURIComponent(new URL(req.url,'http://x').pathname));
 if(!f.startsWith(path.resolve(root))||!fs.existsSync(f)||!fs.statSync(f).isFile()){res.writeHead(404);res.end();return;}
 res.setHeader('Content-Type',types[path.extname(f)]||'application/octet-stream');fs.createReadStream(f).pipe(res);});
const sizes=[[1920,1080],[1280,720],[1366,768],[1600,900],[2560,1440],[1024,768],[800,600],[1280,1024],[3840,2160],[1680,1050]];
// name, search text, selected track, list scroll
const states=[['start','',0,0],['search','ri',1,0],['picked','',6,200]];
const elements={label:'#offline-screen header .label',h1:'#offline-screen h1',back:'#offline-back',
 stepMap:'#offline-step-map',stepRules:'#offline-step-rules',left:'#offline-track > section:first-child',
 right:'#offline-track > section.showroom',modeLabel:'label[for=offline-mode]',mode:'#offline-mode',
 searchLabel:'label[for=offline-search]',search:'#offline-search',count:'#offline-map-count',list:'#offline-map-list',
 mapId:'#offline-map-id',name:'#offline-map-name',image:'#offline-map-image',details:'#offline-map-details',next:'#offline-next'};
(async()=>{await new Promise(r=>server.listen(0,'127.0.0.1',r));
 const browser=await chromium.launch({headless:true,...(process.env.WEBUI_BROWSER?{executablePath:process.env.WEBUI_BROWSER}:{}),args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:1920,height:1080}});
 await page.goto('http://127.0.0.1:'+server.address().port+'/index.html');
 await page.waitForFunction(()=>!document.querySelector('#apply').disabled);
 await page.evaluate(()=>document.fonts.ready);
 const result={};
 for(const [w,h] of sizes){
  await page.setViewportSize({width:w,height:h});
  for(const [name,query,pick,scroll] of states){
   await page.evaluate(([model,query,pick,scroll])=>{
    q3rallyFrontend.state(0,0,0,0);q3rallyFrontend.state(9,0,0,0);
    q3rallyOffline.begin(9,model.mode,pick,0,model.hosting,0);
    model.modes.forEach((t,i)=>q3rallyOffline.mode(i,t));model.maps.forEach(([id,t],i)=>q3rallyOffline.map(i,id,t));
    model.details.forEach(t=>q3rallyOffline.detail(t));q3rallyOffline.end();
    const search=document.querySelector('#offline-search');search.value=query;search.dispatchEvent(new Event('input'));
    document.querySelector('#offline-map-list').scrollTop=scroll;document.activeElement.blur();scrollTo(0,0);
   },[model,query,pick,scroll]);
   await page.waitForTimeout(shots&&(w===1920||w===800)?1500:300);
   result[`${w}x${h}_${name}`]=await page.evaluate(elements=>{
    const round=v=>Math.round(v*1000)/1000, box=e=>{const r=e.getBoundingClientRect();return [r.left,r.top,r.width,r.height].map(round);};
    const els={};
    for(const [key,selector] of Object.entries(elements)){const e=document.querySelector(selector);if(e&&e.getClientRects().length)els[key]=box(e);}
    const list=document.querySelector('#offline-map-list');
    els.buttons=[...list.querySelectorAll('button')].map(b=>[...box(b),b.getAttribute('aria-pressed')==='true'?1:0]);
    els.thumbs=[...list.querySelectorAll('img')].map(box);
    els.detailLines=[...document.querySelectorAll('#offline-map-details p')].map(box);
    els.scroll=[list.scrollTop,list.scrollHeight,list.clientWidth,list.clientHeight];
    const texts=[];
    const walker=document.createTreeWalker(document.querySelector('#offline-screen'),NodeFilter.SHOW_TEXT);let node;
    while((node=walker.nextNode())){
     const e=node.parentElement;if(!node.textContent.trim()||!e.getClientRects().length)continue;
     const style=getComputedStyle(e),range=document.createRange();range.selectNodeContents(node);
     texts.push({text:node.textContent,family:style.fontFamily.split(',')[0].replace(/"/g,'').replace(/'/g,''),size:parseFloat(style.fontSize),
      weight:+style.fontWeight,italic:style.fontStyle==='italic',spacing:parseFloat(style.letterSpacing)||0,
      uppercase:style.textTransform==='uppercase',color:style.color,lines:[...range.getClientRects()].map(r=>[r.left,r.top,r.width,r.height].map(round))});
    }
    return {els,texts,height:document.documentElement.getBoundingClientRect().height};
   },elements);
   if(shots&&(w===1920||w===800))await page.screenshot({path:path.join(shots,`weboffline_${w}x${h}_${name}.png`)});
  }
 }
 fs.writeFileSync(output,JSON.stringify(result)+'\n');
 await browser.close();server.close();})();
