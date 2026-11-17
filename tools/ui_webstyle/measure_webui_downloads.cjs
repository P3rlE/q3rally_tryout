// Measure the WebUI's Downloads screen (screen 6) in Chrome: the reference the
// native Downloads screen (q3_ui/ui_rally_rivalslayout.c, downloads mode) is
// tested against.
//
//   node tools/ui_webstyle/measure_webui_downloads.cjs <staged garage> <fixture.json> [screenshot dir]
//
// The catalog is tests/fixtures/downloads_index.json, published as the client
// does it (Web_PublishCatalog); entries whose id ends in "_2" or "rev" count as
// installed. See measure_webui_loading.cjs for the staged garage.
const {chromium}=require('playwright');const fs=require('fs'),path=require('path'),http=require('http');
const [root,output,shots]=process.argv.slice(2);
const repo=path.resolve(__dirname,'../..');
const catalog=JSON.parse(fs.readFileSync(path.join(repo,'tests/fixtures/downloads_index.json'),'utf8'));
const items=catalog.map(e=>[[e.name,e.author,e.type,e.version,e.id],Math.floor(e.size/1024),/(_2|rev)$/.test(e.id)?1:0]);
const types={'.html':'text/html','.js':'text/javascript','.css':'text/css','.json':'application/json','.png':'image/png','.webp':'image/webp','.glb':'model/gltf-binary','.ttf':'font/ttf'};
const server=http.createServer((req,res)=>{const f=path.resolve(root,'.'+decodeURIComponent(new URL(req.url,'http://x').pathname));
 if(!f.startsWith(path.resolve(root))||!fs.existsSync(f)||!fs.statSync(f).isFile()){res.writeHead(404);res.end();return;}
 res.setHeader('Content-Type',types[path.extname(f)]||'application/octet-stream');fs.createReadStream(f).pipe(res);});
const sizes=[[1920,1080],[1280,720],[1366,768],[1600,900],[2560,1440],[1024,768],[800,600],[1280,1024],[3840,2160],[1680,1050]];
// name, search text, type filter, item to select, download state, progress
const states=[['start','','',null,2,0],['filter','','track',null,2,0],['search','ri','',null,2,0],
 ['downloading','','',2,3,42],['installed','','',4,4,100]];
const elements={h1:'#library-screen h1',back:'#library-back',left:'.library-layout > section:first-child',
 typeLabel:'label[for=download-type]',type:'#download-type',install:'#download-install',refresh:'#download-refresh',
 cancel:'#download-cancel',progress:'#download-progress',downloadStatus:'#download-status',image:'#library-image',
 right:'.library-layout > section.showroom',searchLabel:'label[for=library-search]',search:'#library-search',
 count:'#library-count',list:'#library-list',detailLabel:'#library-detail-label',name:'#library-name',car:'#rival-3d',
 author:'#library-author',personality:'#library-personality',weapon:'#library-weapon',description:'#library-description',
 status:'#library-detail-status'};
(async()=>{await new Promise(r=>server.listen(0,'127.0.0.1',r));
 // SCROLLBARS=1 shows them as CEF does (for the look of the list's scrollbar);
 // the layout is measured without, like the other screens
 const browser=await chromium.launch({headless:true,...(process.env.SCROLLBARS?{ignoreDefaultArgs:['--hide-scrollbars']}:{}),...(process.env.WEBUI_BROWSER?{executablePath:process.env.WEBUI_BROWSER}:{}),args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:1920,height:1080}});
 await page.goto('http://127.0.0.1:'+server.address().port+'/index.html');
 await page.waitForFunction(()=>!document.querySelector('#apply').disabled);
 await page.evaluate(()=>document.fonts.ready);
 const result={items};
 for(const [w,h] of sizes){
  await page.setViewportSize({width:w,height:h});
  for(const [name,query,type,pick,state,progress] of states){
   await page.evaluate(([items,query,type,pick,state,progress])=>{
    q3rallyFrontend.state(0,0,0,0);q3rallyFrontend.state(6,0,0,0);
    q3rallyLibrary.begin(6);items.forEach(([fields,size,installed],i)=>q3rallyLibrary.item(6,i,fields,size,installed));q3rallyLibrary.end(6);
    const search=document.querySelector('#library-search');search.value=query;
    document.querySelector('#download-type').value=type;search.dispatchEvent(new Event('input'));
    if(pick!==null)document.querySelector(`#library-list button[data-index="${pick}"]`).click();
    q3rallyLibrary.downloadState(state,progress,'trk_night_city_x.pk3','');
    document.querySelector('#library-image').hidden=true;
    document.activeElement.blur();scrollTo(0,0);
   },[items,query,type,pick,state,progress]);
   await page.waitForTimeout(shots&&(w===1920||w===800)?3000:300);
   result[`${w}x${h}_${name}`]=await page.evaluate(elements=>{
    const round=v=>Math.round(v*1000)/1000, box=e=>{const r=e.getBoundingClientRect();return [r.left,r.top,r.width,r.height].map(round);};
    const els={};
    for(const [key,selector] of Object.entries(elements)){const e=document.querySelector(selector);if(e&&e.getClientRects().length)els[key]=box(e);}
    const list=document.querySelector('#library-list');
    els.buttons=[...list.querySelectorAll('button')].map(b=>[+b.dataset.index,...box(b),b.getAttribute('aria-pressed')==='true'?1:0]);
    els.icons=[...list.querySelectorAll('img')].map(box);
    els.weaponIcon=[...document.querySelectorAll('#library-weapon img')].map(box);
    els.scroll=[list.scrollTop,list.scrollHeight,list.clientWidth,list.clientHeight];els.page=[scrollX,scrollY,document.documentElement.clientWidth];
    const texts=[];
    const walker=document.createTreeWalker(document.querySelector('#library-screen'),NodeFilter.SHOW_TEXT);let node;
    while((node=walker.nextNode())){
     const e=node.parentElement;if(!node.textContent.trim()||!e.getClientRects().length||e.closest('[hidden]'))continue;
     const style=getComputedStyle(e),range=document.createRange();range.selectNodeContents(node);
     texts.push({text:node.textContent,family:style.fontFamily.split(',')[0].replace(/"/g,'').replace(/'/g,''),size:parseFloat(style.fontSize),
      weight:+style.fontWeight,italic:style.fontStyle==='italic',spacing:parseFloat(style.letterSpacing)||0,
      uppercase:style.textTransform==='uppercase',color:style.color,lines:[...range.getClientRects()].map(r=>[r.left,r.top,r.width,r.height].map(round))});
    }
    return {els,texts,height:document.documentElement.getBoundingClientRect().height};
   },elements);
   if(shots&&(w===1920||w===800))await page.screenshot({path:path.join(shots,`webdownloads_${w}x${h}_${name}.png`)});
  }
 }
 fs.writeFileSync(output,JSON.stringify(result)+'\n');
 await browser.close();server.close();})();
