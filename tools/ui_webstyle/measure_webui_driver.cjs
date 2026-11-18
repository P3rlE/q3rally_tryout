// Measure the WebUI's driver page (#driver-screen, screen 8) and its dialog
// in Chrome: the reference the native one (q3_ui/ui_rally_driver.c) is
// tested against.
//
//   node tools/ui_webstyle/measure_webui_driver.cjs <staged garage> <fixture.json> [screenshot dir]
//
// The data is the game's own: tests/fixtures/driver_model.json holds what the
// UI prints with ui_driverDump 1 for a driver with progress (and two profiles
// on disk), and the same without an active profile. Every visible element is
// recorded under a path (tag#id.class:index from the screen or the dialog
// down), with the text runs when TEXTS=1.
const {chromium}=require('playwright');const fs=require('fs'),path=require('path'),http=require('http');
const [root,output,shots]=process.argv.slice(2);
const repo=path.resolve(__dirname,'../..');
const models=JSON.parse(fs.readFileSync(path.join(repo,'tests/fixtures/driver_model.json'),'utf8'));
const types={'.html':'text/html','.js':'text/javascript','.css':'text/css','.json':'application/json','.png':'image/png','.webp':'image/webp','.glb':'model/gltf-binary','.ttf':'font/ttf'};
const server=http.createServer((req,res)=>{const f=path.resolve(root,'.'+decodeURIComponent(new URL(req.url,'http://x').pathname));
 if(!f.startsWith(path.resolve(root))||!fs.existsSync(f)||!fs.statSync(f).isFile()){res.writeHead(404);res.end();return;}
 res.setHeader('Content-Type',types[path.extname(f)]||'application/octet-stream');fs.createReadStream(f).pipe(res);});
const sizes=[[1920,1080],[1280,720],[1366,768],[1600,900],[2560,1440],[1024,768],[800,600],[1280,1024],[3840,2160],[1680,1050]];
// name, model, tab, achievement filter, dialog
const states=[['overview','driver',0,'',''],['stats','driver',1,'',''],['achievements','driver',2,'',''],['filtered','driver',2,'2',''],
 ['empty','empty',0,'',''],['edit','driver',0,'','edit'],['switch','driver',0,'','switch']];
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
  const withTexts=!!process.env.TEXTS&&(w===1920||w===1280||w===800), shot=w===1920||w===1280||w===800;
  for(const [name,key,tab,filter,dialog] of states){
   const model=models[key];
   await page.evaluate(([model,tab,filter,dialog])=>{
    const d=document.querySelector('#driver-dialog');if(d?.open)d.close();
    q3rallyFrontend.state(0,0,0,0);q3rallyFrontend.state(8,0,0,0);
    q3rallyDriver.begin();model.available.forEach(n=>q3rallyDriver.available(n));
    if(model.profile)q3rallyDriver.profile(model.profile);
    q3rallyDriver.rank(...model.rank);
    model.categories.forEach(([title,progress,tiers],i)=>{q3rallyDriver.category(i,title,String(progress));
     tiers.forEach(t=>q3rallyDriver.tier(i,...t));});
    q3rallyDriver.end(tab);
    const select=document.querySelector('#driver-achievement-filter');select.value=filter;select.dispatchEvent(new Event('change'));
    if(dialog)document.querySelector(dialog==='edit'?'#driver-overview .driver-actions button:nth-child(1)':'#driver-overview .driver-actions button:nth-child(2)').click();
    document.activeElement.blur();scrollTo(0,0);
   },[model,tab,filter,dialog]);
   await page.waitForTimeout(shots&&shot?1200:250);
   result[`${w}x${h}_${name}`]=await page.evaluate(withTexts=>{
    const round=v=>Math.round(v*1000)/1000, box=e=>{const r=e.getBoundingClientRect();return [r.left,r.top,r.width,r.height].map(round);};
    const els={};
    const walk=(e,prefix)=>{[...e.children].forEach((c,i)=>{
     if(!c.getClientRects().length||c.tagName==='OPTION')return;
     const key=prefix+'/'+c.tagName.toLowerCase()+(c.id?'#'+c.id:'')+(c.className&&typeof c.className==='string'?'.'+c.className.trim().split(/\s+/).join('.'):'')+':'+i;
     els[key]=box(c);walk(c,key);});};
    const screen=document.querySelector('#driver-screen'),dialog=document.querySelector('#driver-dialog');
    walk(screen,'');if(dialog.open){els['dialog']=box(dialog);walk(dialog,'dialog');}
    const texts=[];
    for(const top of [screen,dialog]){
     const walker=document.createTreeWalker(top,NodeFilter.SHOW_TEXT);let node;
     while(withTexts&&(node=walker.nextNode())){
      const e=node.parentElement;if(!node.textContent.trim()||!e.getClientRects().length||e.tagName==='OPTION')continue;
      const style=getComputedStyle(e),range=document.createRange();range.selectNodeContents(node);
      texts.push({text:node.textContent,family:style.fontFamily.split(',')[0].replace(/"/g,'').replace(/'/g,''),size:parseFloat(style.fontSize),
       weight:+style.fontWeight,italic:style.fontStyle==='italic',spacing:parseFloat(style.letterSpacing)||0,
       uppercase:style.textTransform==='uppercase',color:style.color,lines:[...range.getClientRects()].map(r=>[r.left,r.top,r.width,r.height].map(round))});
     }
    }
    const list=document.querySelector('.driver-profile-list');
    return {els,texts,listScroll:list?[list.scrollTop,list.scrollHeight,list.clientWidth,list.clientHeight]:null,
     selects:[...document.querySelectorAll('#driver-screen select,#driver-dialog select')].filter(s=>s.getClientRects().length).map(s=>[s.options[s.selectedIndex]?.text??'',...box(s)]),
     height:document.documentElement.getBoundingClientRect().height};
   },withTexts);
   if(shots&&shot)await page.screenshot({path:path.join(shots,`webdriver_${w}x${h}_${name}.png`)});
  }
 }
 fs.writeFileSync(output,JSON.stringify(result)+'\n');
 await browser.close();server.close();})();
