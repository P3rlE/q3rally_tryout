// Measure the WebUI's settings (#config-screen, screen 12: config pages
// 0-21, the wizards included) in Chrome: the reference the native ones (q3_ui/ui_rally_config.c)
// are tested against.
//
//   node tools/ui_webstyle/measure_webui_config.cjs <staged garage> <fixture.json> [screenshot dir]
//
// The data is the game's own: tests/fixtures/config_model.json holds the
// pages as the UI prints them with ui_configDump 1 (one made-up page with
// the two list items, the details step with an avatar chosen). Every visible element is recorded under a path
// (tag#id.class:index from the screen down, "dialog/..." for an open dialog).
const {chromium}=require('playwright');const fs=require('fs'),path=require('path'),http=require('http');
const [root,output,shots]=process.argv.slice(2);
const repo=path.resolve(__dirname,'../..');
const models=JSON.parse(fs.readFileSync(path.join(repo,'tests/fixtures/config_model.json'),'utf8'));
const types={'.html':'text/html','.js':'text/javascript','.css':'text/css','.json':'application/json','.png':'image/png','.webp':'image/webp','.glb':'model/gltf-binary','.ttf':'font/ttf'};
const server=http.createServer((req,res)=>{const f=path.resolve(root,'.'+decodeURIComponent(new URL(req.url,'http://x').pathname));
 if(!f.startsWith(path.resolve(root))||!fs.existsSync(f)||!fs.statSync(f).isFile()){res.writeHead(404);res.end();return;}
 res.setHeader('Content-Type',types[path.extname(f)]||'application/octet-stream');fs.createReadStream(f).pipe(res);});
const sizes=[[1920,1080],[1280,720],[1366,768],[1600,900],[2560,1440],[1024,768],[800,600],[1280,1024],[3840,2160],[1680,1050]];
const states=Object.keys(models).map(k=>[k,k,false]).concat([['capture','controls',true]]);
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
  const shot=w===1920||w===1280||w===800;
  for(const [name,key,capture] of states){
   const model=models[key];
   await page.evaluate(([model,capture])=>{
    document.querySelectorAll('#config-screen dialog').forEach(d=>d.open&&d.close());
    q3rallyFrontend.state(0,0,0,0);q3rallyFrontend.state(12,0,0,0);
    q3rallyConfig.begin(-1,'','',0);q3rallyConfig.end();
    q3rallyConfig.begin(model.page,model.title,model.description,model.confirmation);
    model.items.forEach(([index,type,label,text,value,min,max,disabled,choices])=>{q3rallyConfig.item(index,type,label,text,value,min,max,disabled);
     choices.forEach(([c,image],j)=>q3rallyConfig.choice(index,j,c,image));});
    q3rallyConfig.end();
    if(capture)document.querySelector('#config-fields .config-actions button').click();
    document.activeElement?.blur?.();scrollTo(0,0);
   },[model,capture]);
   await page.waitForTimeout(shots&&shot?1000:200);
   result[`${w}x${h}_${name}`]=await page.evaluate(()=>{
    const round=v=>Math.round(v*1000)/1000, box=e=>{const r=e.getBoundingClientRect();return [r.left,r.top,r.width,r.height].map(round);};
    const els={};
    const walk=(e,prefix)=>{[...e.children].forEach((c,i)=>{
     if(!c.getClientRects().length||c.tagName==='OPTION')return;
     const key=prefix+'/'+c.tagName.toLowerCase()+(c.id?'#'+c.id:'')+(c.className&&typeof c.className==='string'?'.'+c.className.trim().split(/\s+/).join('.'):'')+':'+i;
     els[key]=box(c);if(c.tagName!=='DIALOG')walk(c,key);});};
    const screen=document.querySelector('#config-screen');walk(screen,'');
    const dialog=[...screen.querySelectorAll('dialog')].find(d=>d.open);
    if(dialog){els['dialog']=box(dialog);walk(dialog,'dialog');}
    const selects=[...screen.querySelectorAll('select')].filter(s=>s.getClientRects().length).map(s=>[s.options[s.selectedIndex]?.text??'',...box(s)]);
    return {els,selects,height:document.documentElement.getBoundingClientRect().height};
   });
   if(shots&&shot)await page.screenshot({path:path.join(shots,`webconfig_${w}x${h}_${name}.png`)});
  }
 }
 fs.writeFileSync(output,JSON.stringify(result)+'\n');
 await browser.close();server.close();})();
