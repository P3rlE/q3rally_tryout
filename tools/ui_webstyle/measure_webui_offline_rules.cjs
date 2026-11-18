// Measure step 2 of the WebUI's offline race setup (screen 10, rules and
// drivers) in Chrome: the reference the native one
// (q3_ui/ui_rally_offlinerules.c) is tested against.
//
//   node tools/ui_webstyle/measure_webui_offline_rules.cjs <staged garage> <fixture.json> [screenshot dir]
//
// The data is the game's own: tests/fixtures/offline_rules_model.json, as the
// UI prints it with ui_offlineDump 1 on step 2 (Racing, Team Racing, Drag
// Race). See measure_webui_offline.cjs.
const {chromium}=require('playwright');const fs=require('fs'),path=require('path'),http=require('http');
const [root,output,shots]=process.argv.slice(2);
const repo=path.resolve(__dirname,'../..');
const models=JSON.parse(fs.readFileSync(path.join(repo,'tests/fixtures/offline_rules_model.json'),'utf8'));
const types={'.html':'text/html','.js':'text/javascript','.css':'text/css','.json':'application/json','.png':'image/png','.webp':'image/webp','.glb':'model/gltf-binary','.ttf':'font/ttf'};
const server=http.createServer((req,res)=>{const f=path.resolve(root,'.'+decodeURIComponent(new URL(req.url,'http://x').pathname));
 if(!f.startsWith(path.resolve(root))||!fs.existsSync(f)||!fs.statSync(f).isFile()){res.writeHead(404);res.end();return;}
 res.setHeader('Content-Type',types[path.extname(f)]||'application/octet-stream');fs.createReadStream(f).pipe(res);});
const sizes=[[1920,1080],[1280,720],[1366,768],[1600,900],[2560,1440],[1024,768],[800,600],[1280,1024],[3840,2160],[1680,1050]];
// name, model, avatar, options scroll, players scroll
const states=[['racing','racing','',0,0],['avatar','racing','gfx/avatars/preset/helmet_01',0,120],['team','team','',0,0],['drag','drag','',0,0]];
const elements={label:'#offline-screen header .label',h1:'#offline-screen h1',back:'#offline-back',
 stepMap:'#offline-step-map',stepRules:'#offline-step-rules',summary:'.offline-summary',summaryLabel:'.offline-summary .label',
 title:'#offline-race-title',raceMode:'#offline-race-mode',start:'#offline-start',
 left:'.offline-rules-layout > section:first-child',right:'#offline-opponents',
 rulesH2:'.offline-rules-layout > section:first-child h2',ruleHelp:'#offline-rule-help',options:'#offline-options',
 gridH2:'#offline-opponents h2',gridHelp:'#offline-grid-help',players:'#offline-players',status:'#offline-status'};
(async()=>{await new Promise(r=>server.listen(0,'127.0.0.1',r));
 const browser=await chromium.launch({headless:true,...(process.env.WEBUI_BROWSER?{executablePath:process.env.WEBUI_BROWSER}:{}),args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:1920,height:1080}});
 await page.goto('http://127.0.0.1:'+server.address().port+'/index.html');
 await page.waitForFunction(()=>!document.querySelector('#apply').disabled);
 await page.evaluate(()=>document.fonts.ready);
 const result={};
 for(const [w,h] of sizes){
  await page.setViewportSize({width:w,height:h});
  for(const [name,key,avatar,optionsScroll,playersScroll] of states){
   const model=models[key];
   await page.evaluate(([model,avatar,optionsScroll,playersScroll])=>{
    q3rallyFrontend.state(0,0,0,0);q3rallyFrontend.state(10,0,0,0);
    q3rallyOffline.begin(10,model.mode,model.selected,model.allowsBots,model.hosting,model.dedicated);
    model.modes.forEach((t,i)=>q3rallyOffline.mode(i,t));model.maps.forEach(([id,t],i)=>q3rallyOffline.map(i,id,t));
    model.details.forEach(t=>q3rallyOffline.detail(t));q3rallyOffline.avatar(avatar||model.avatar);
    model.controls.forEach(([index,type,label,text,value,disabled,slot,team,maxchars,choices])=>{
     q3rallyOffline.control(index,type,label,text,value,disabled,slot,team,maxchars);choices.forEach((c,j)=>q3rallyOffline.choice(index,j,c));});
    model.players.forEach((p,i)=>q3rallyOffline.player(i,p));model.bots.forEach(([n,m],i)=>q3rallyOffline.bot(i,n,m));
    q3rallyOffline.end();
    document.querySelector('#offline-options').scrollTop=optionsScroll;document.querySelector('#offline-players').scrollTop=playersScroll;
    document.activeElement.blur();scrollTo(0,0);
   },[model,avatar,optionsScroll,playersScroll]);
   await page.waitForTimeout(shots&&(w===1920||w===800||w===1280)?1500:300);
   result[`${w}x${h}_${name}`]=await page.evaluate(elements=>{
    const round=v=>Math.round(v*1000)/1000, box=e=>{const r=e.getBoundingClientRect();return [r.left,r.top,r.width,r.height].map(round);};
    const els={};
    for(const [key,selector] of Object.entries(elements)){const e=document.querySelector(selector);if(e&&e.getClientRects().length)els[key]=box(e);}
    els.fields=[...document.querySelectorAll('#offline-options > label')].map(l=>({label:box(l),control:box(l.querySelector('input,select'))}));
    els.rows=[...document.querySelectorAll('#offline-players > .offline-player')].map(row=>({row:box(row),
     parts:[...row.children].map(e=>e.tagName==='LABEL'?['label',box(e),box(e.querySelector('select,input'))]:[e.tagName.toLowerCase(),box(e)])}));
    const o=document.querySelector('#offline-options'),p=document.querySelector('#offline-players');
    els.optionsScroll=[o.scrollTop,o.scrollHeight,o.clientWidth,o.clientHeight];
    els.playersScroll=[p.scrollTop,p.scrollHeight,p.clientWidth,p.clientHeight];
    const texts=[];
    const walker=document.createTreeWalker(document.querySelector('#offline-screen'),NodeFilter.SHOW_TEXT);let node;
    while((node=walker.nextNode())){
     const e=node.parentElement;if(!node.textContent.trim()||!e.getClientRects().length||e.tagName==='OPTION')continue;
     const style=getComputedStyle(e),range=document.createRange();range.selectNodeContents(node);
     texts.push({text:node.textContent,family:style.fontFamily.split(',')[0].replace(/"/g,'').replace(/'/g,''),size:parseFloat(style.fontSize),
      weight:+style.fontWeight,italic:style.fontStyle==='italic',spacing:parseFloat(style.letterSpacing)||0,
      uppercase:style.textTransform==='uppercase',color:style.color,lines:[...range.getClientRects()].map(r=>[r.left,r.top,r.width,r.height].map(round))});
    }
    const selects=[...document.querySelectorAll('#offline-rules select')].filter(s=>s.getClientRects().length).map(s=>[box(s),s.options[s.selectedIndex]?.text??'']);
    const inputs=[...document.querySelectorAll('#offline-rules input')].map(i=>[box(i),i.value]);
    return {els,texts,selects,inputs,height:document.documentElement.getBoundingClientRect().height};
   },elements);
   if(shots&&(w===1920||w===800||w===1280))await page.screenshot({path:path.join(shots,`webrules_${w}x${h}_${name}.png`)});
  }
 }
 fs.writeFileSync(output,JSON.stringify(result)+'\n');
 await browser.close();server.close();})();
