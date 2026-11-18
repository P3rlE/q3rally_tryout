// Measure the WebUI's server browser (#online-screen, screen 11) in Chrome:
// the reference the native one (q3_ui/ui_rally_online.c) is tested against.
//
//   node tools/ui_webstyle/measure_webui_online.cjs <staged garage> <fixture.json> [screenshot dir]
//
// The data is the game's own: tests/fixtures/online_model.json holds the
// model as the UI prints it with ui_onlineDump 1 (three local servers saved as
// favorites and one that does not answer), and the same list grown to 14
// servers for a scrolled list. Every visible element is recorded under a path
// (tag#id.class:index from #online-screen down), with the text runs.
const {chromium}=require('playwright');const fs=require('fs'),path=require('path'),http=require('http');
const [root,output,shots]=process.argv.slice(2);
const repo=path.resolve(__dirname,'../..');
const models=JSON.parse(fs.readFileSync(path.join(repo,'tests/fixtures/online_model.json'),'utf8'));
const types={'.html':'text/html','.js':'text/javascript','.css':'text/css','.json':'application/json','.png':'image/png','.webp':'image/webp','.glb':'model/gltf-binary','.ttf':'font/ttf'};
const server=http.createServer((req,res)=>{const f=path.resolve(root,'.'+decodeURIComponent(new URL(req.url,'http://x').pathname));
 if(!f.startsWith(path.resolve(root))||!fs.existsSync(f)||!fs.statSync(f).isFile()){res.writeHead(404);res.end();return;}
 res.setHeader('Content-Type',types[path.extname(f)]||'application/octet-stream');fs.createReadStream(f).pipe(res);});
const sizes=[[1920,1080],[1280,720],[1366,768],[1600,900],[2560,1440],[1024,768],[800,600],[1280,1024],[3840,2160],[1680,1050]];
// name, model, selected, query, list scroll, scanning, dialog, input
const states=[['start','favorites','','',0,0,'',''],['picked','favorites','127.0.0.1:27962','',0,0,'',''],
 ['scrolled','internet','[2001:db8::1]:27960','',300,0,'',''],['search','favorites','','ridge',0,0,'',''],
 ['nomatch','favorites','','zzz',0,0,'',''],['scanning','favorites','','',0,1,'',''],
 ['direct','favorites','','',0,0,'direct','abc'],['remove','favorites','127.0.0.1:27961','',0,0,'remove','']];
(async()=>{await new Promise(r=>server.listen(0,'127.0.0.1',r));
 const browser=await chromium.launch({headless:true,...(process.env.WEBUI_BROWSER?{executablePath:process.env.WEBUI_BROWSER}:{}),args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:1920,height:1080}});
 await page.goto('http://127.0.0.1:'+server.address().port+'/index.html');
 await page.waitForFunction(()=>!document.querySelector('#apply').disabled);
 await page.evaluate(()=>document.fonts.ready);
 const result={};
 for(const [w,h] of sizes){
  await page.setViewportSize({width:w,height:h});
  for(const [name,key,selected,query,listScroll,scanning,dialog,input] of states){
   const model=models[key];
   await page.evaluate(([model,selected,query,listScroll,scanning,dialog,input])=>{
    document.querySelectorAll('#online-screen dialog').forEach(d=>d.open&&d.close());
    q3rallyFrontend.state(0,0,0,0);q3rallyFrontend.state(11,0,0,0);
    const f=model.filters;q3rallyOnline.begin(f[0],f[1],f[2],f[3],f[4],scanning?0:f[5],scanning);
    for(const [kind,titles] of Object.entries(model.options))titles.forEach((t,i)=>q3rallyOnline.option(+kind,i,t));
    if(!scanning)for(const s of model.servers)q3rallyOnline.server(...s);
    document.querySelector('#online-search').value=query;
    q3rallyOnline.end(scanning?'Searching for servers...':model.status,dialog==='direct'?'':model.message,selected);
    document.querySelector('#online-list').scrollTop=listScroll;
    if(dialog==='direct'){document.querySelector('#online-direct').click();document.querySelector('#online-direct-address').value=input;
     document.querySelector('#online-direct-error').textContent='Enter a valid server address and port (1-65535).';}
    if(dialog==='remove')document.querySelector('#online-remove').click();
    document.activeElement.blur();scrollTo(0,0);
   },[model,selected,query,listScroll,scanning,dialog,input]);
   await page.waitForTimeout(shots&&(w===1920||w===800||w===1280)?1200:250);
   const withTexts=w===1920||w===1280||w===800;
   result[`${w}x${h}_${name}`]=await page.evaluate(withTexts=>{
    const round=v=>Math.round(v*1000)/1000, box=e=>{const r=e.getBoundingClientRect();return [r.left,r.top,r.width,r.height].map(round);};
    const root=document.querySelector('#online-screen'),els={};
    const walk=(e,prefix)=>{[...e.children].forEach((c,i)=>{
     if(!c.getClientRects().length||c.tagName==='OPTION')return;
     const key=prefix+'/'+c.tagName.toLowerCase()+(c.id?'#'+c.id:'')+(c.className&&typeof c.className==='string'?'.'+c.className.trim().split(/\s+/).join('.'):'')+':'+i;
     els[key]=box(c);walk(c,key);});};
    walk(root,'');
    const texts=[];
    const walker=document.createTreeWalker(root,NodeFilter.SHOW_TEXT);let node;
    while(withTexts&&(node=walker.nextNode())){
     const e=node.parentElement;if(!node.textContent.trim()||!e.getClientRects().length||e.tagName==='OPTION')continue;
     const style=getComputedStyle(e),range=document.createRange();range.selectNodeContents(node);
     texts.push({text:node.textContent,family:style.fontFamily.split(',')[0].replace(/"/g,'').replace(/'/g,''),size:parseFloat(style.fontSize),
      weight:+style.fontWeight,italic:style.fontStyle==='italic',spacing:parseFloat(style.letterSpacing)||0,
      uppercase:style.textTransform==='uppercase',color:style.color,opacity:style.opacity,lines:[...range.getClientRects()].map(r=>[r.left,r.top,r.width,r.height].map(round))});
    }
    const list=document.querySelector('#online-list');
    return {els,texts,listScroll:[list.scrollTop,list.scrollHeight,list.clientWidth,list.clientHeight],
     height:document.documentElement.getBoundingClientRect().height};
   },withTexts);
   if(shots&&(w===1920||w===800||w===1280))await page.screenshot({path:path.join(shots,`webonline_${w}x${h}_${name}.png`)});
  }
 }
 fs.writeFileSync(output,JSON.stringify(result)+'\n');
 await browser.close();server.close();})();
