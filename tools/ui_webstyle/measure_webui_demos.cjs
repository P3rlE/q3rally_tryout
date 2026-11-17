// Measure the WebUI's demo archive (screen 4) in Chrome: the reference the
// native demo archive (q3_ui/ui_rally_demoslayout.c) is tested against.
//
//   node tools/ui_webstyle/measure_webui_demos.cjs <staged garage> <fixture.json> [screenshot dir]
//
// The recordings are tests/fixtures/demo_names.json (name, bytes), published
// as the client does it (Web_PublishItems). See measure_webui_loading.cjs.
const {chromium}=require('playwright');const fs=require('fs'),path=require('path'),http=require('http');
const [root,output,shots]=process.argv.slice(2);
const repo=path.resolve(__dirname,'../..');
const demos=JSON.parse(fs.readFileSync(path.join(repo,'tests/fixtures/demo_names.json'),'utf8'));
const types={'.html':'text/html','.js':'text/javascript','.css':'text/css','.json':'application/json','.png':'image/png','.webp':'image/webp','.glb':'model/gltf-binary','.ttf':'font/ttf'};
const server=http.createServer((req,res)=>{const f=path.resolve(root,'.'+decodeURIComponent(new URL(req.url,'http://x').pathname));
 if(!f.startsWith(path.resolve(root))||!fs.existsSync(f)||!fs.statSync(f).isFile()){res.writeHead(404);res.end();return;}
 res.setHeader('Content-Type',types[path.extname(f)]||'application/octet-stream');fs.createReadStream(f).pipe(res);});
const sizes=[[1920,1080],[1280,720],[1366,768],[1600,900],[2560,1440],[1024,768],[800,600],[1280,1024],[3840,2160],[1680,1050]];
// name, recording to select, playback (active, frozen, seeking, seconds, message)
const states=[['start',0,[0,0,0,0,'']],['playing',2,[1,0,0,75,'']],['paused',11,[1,1,0,3725,'']]];
const elements={h1:'#demos-screen h1',back:'#demos-back',left:'.archive-layout > section:first-child',
 right:'.archive-layout > section:last-child',recordings:'.archive-layout > section:first-child > .label',
 choose:'label[for=demo-list]',list:'#demo-list',details:'#demo-details',play:'#demo-play',
 preview:'.archive-layout > section:last-child > .label',viewport:'#demo-viewport',placeholder:'#demo-placeholder',
 transport:'.demo-transport',clockLine:'#demo-clock',message:'#demo-message'};
(async()=>{await new Promise(r=>server.listen(0,'127.0.0.1',r));
 // SCROLLBARS=1 shows them as CEF does (for the look of the list's scrollbar);
 // the layout is measured without, like the other screens
 const browser=await chromium.launch({headless:true,...(process.env.SCROLLBARS?{ignoreDefaultArgs:['--hide-scrollbars']}:{}),...(process.env.WEBUI_BROWSER?{executablePath:process.env.WEBUI_BROWSER}:{}),args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:1920,height:1080}});
 await page.goto('http://127.0.0.1:'+server.address().port+'/index.html');
 await page.waitForFunction(()=>!document.querySelector('#apply').disabled);
 await page.evaluate(()=>document.fonts.ready);
 const result={demos};
 for(const [w,h] of sizes){
  await page.setViewportSize({width:w,height:h});
  for(const [name,pick,playback] of states){
   await page.evaluate(([demos,pick,playback])=>{
    q3rallyFrontend.state(0,0,0,0);q3rallyFrontend.state(4,0,0,0);
    q3rallyArchive.demosBegin();demos.forEach(([text,size],i)=>q3rallyArchive.item(4,i,text,size,0));q3rallyArchive.demosEnd();
    const list=document.querySelector('#demo-list');list.selectedIndex=pick;list.dispatchEvent(new Event('change'));
    q3rallyArchive.playback(...playback);document.activeElement.blur();scrollTo(0,0);
   },[demos,pick,playback]);
   await page.waitForTimeout(shots&&(w===1920||w===800)?3000:300);
   result[`${w}x${h}_${name}`]=await page.evaluate(elements=>{
    const round=v=>Math.round(v*1000)/1000, box=e=>{const r=e.getBoundingClientRect();return [r.left,r.top,r.width,r.height].map(round);};
    const els={};
    for(const [key,selector] of Object.entries(elements)){const e=document.querySelector(selector);if(e&&e.getClientRects().length)els[key]=box(e);}
    const list=document.querySelector('#demo-list');
    els.options=[...list.options].map(o=>box(o));
    els.transportButtons=[...document.querySelectorAll('.demo-transport button')].map(b=>[...box(b),b.disabled?1:0,b.textContent]);
    els.scroll=[list.scrollTop,list.scrollHeight,list.clientWidth,list.clientHeight];els.page=[scrollX,scrollY,document.documentElement.clientWidth];
    const texts=[];
    const walker=document.createTreeWalker(document.querySelector('#demos-screen'),NodeFilter.SHOW_TEXT);let node;
    while((node=walker.nextNode())){
     const e=node.parentElement;if(!node.textContent.trim()||!e.getClientRects().length||e.closest('[hidden]'))continue;
     const style=getComputedStyle(e),range=document.createRange();range.selectNodeContents(node);
     texts.push({text:node.textContent,family:style.fontFamily.split(',')[0].replace(/"/g,'').replace(/'/g,''),size:parseFloat(style.fontSize),
      weight:+style.fontWeight,italic:style.fontStyle==='italic',spacing:parseFloat(style.letterSpacing)||0,
      uppercase:style.textTransform==='uppercase',color:style.color,lines:[...range.getClientRects()].map(r=>[r.left,r.top,r.width,r.height].map(round))});
    }
    return {els,texts,height:document.documentElement.getBoundingClientRect().height};
   },elements);
   if(shots&&(w===1920||w===800))await page.screenshot({path:path.join(shots,`webdemos_${w}x${h}_${name}.png`)});
  }
 }
 fs.writeFileSync(output,JSON.stringify(result)+'\n');
 await browser.close();server.close();})();
