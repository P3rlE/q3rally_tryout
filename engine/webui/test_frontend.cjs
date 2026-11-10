// Run against the staged bundle, never a development server or website checkout.
const {chromium}=require('playwright');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const http=require('node:http');
const root=path.resolve(process.argv[2]);
const output=path.resolve(process.argv[3]);fs.mkdirSync(output,{recursive:true});
const types={'.html':'text/html','.js':'text/javascript','.css':'text/css','.json':'application/json','.png':'image/png','.webp':'image/webp','.glb':'model/gltf-binary','.ttf':'font/ttf'};
const server=http.createServer((req,res)=>{
 const name=decodeURIComponent(new URL(req.url,'http://localhost').pathname);
 const file=path.resolve(root,'.'+name);
 if(!file.startsWith(root+path.sep)||!fs.existsSync(file)||!fs.statSync(file).isFile()){res.writeHead(404);res.end();return;}
 res.setHeader('Content-Type',types[path.extname(file)]||'application/octet-stream');fs.createReadStream(file).pipe(res);
});
(async()=>{
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
 const browser=await chromium.launch({headless:true,...(process.env.WEBUI_BROWSER ? {executablePath:process.env.WEBUI_BROWSER}:{}),args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const errors=[];let checks=0;
 try{
  const page=await browser.newPage({viewport:{width:1280,height:800}});
  page.on('pageerror',e=>errors.push(e.message));
  await page.goto('http://127.0.0.1:'+server.address().port+'/index.html');
  await page.waitForFunction(()=>!document.querySelector('#apply').disabled);checks++;
  const catalog=JSON.parse(fs.readFileSync(path.join(root,'catalog.json')));
  assert.ok(Object.keys(catalog.vehicles).length>=20);
  assert.ok(Object.values(catalog.vehicles).some(car=>car.rank>0));checks++;
  for(const car of Object.values(catalog.vehicles))for(const paint of Object.values(car.paints))assert.ok(fs.existsSync(path.join(root,paint.image)),paint.image);
  await page.evaluate(()=>{q3rallyFrontend.vehicle('alpine','red','svt_cobra');q3rallyFrontend.state(2,0,0,0);});
  await page.waitForFunction(()=>document.querySelector('#active-vehicle-3d canvas'));checks++;
  await page.screenshot({path:path.join(output,'main.png')});
  await page.evaluate(()=>{const c=q3rallyConfig;c.begin(10,'Pause','The online race continues.',0);c.item(0,6,'Resume','',0,0,0,0);c.item(1,6,'Add bots','',0,0,0,0);c.end();q3rallyFrontend.state(12,0,0,0);});
  assert.equal(await page.getByRole('button',{name:'Resume race',exact:true}).count(),1);checks++;
  await page.evaluate(()=>{const h=q3rallyHUD;h.begin();h.item(0,'Speedometer',1,0);h.choice(0,'Off');h.choice(0,'On');h.item(1,'Unavailable mode',0,1);h.choice(1,'Off');h.end(100,90);q3rallyFrontend.state(13,0,0,0);});
  assert.equal(await page.locator('#config-screen').isHidden(),true);
  assert.equal(await page.locator('#hud-item-1').isDisabled(),true);
  assert.ok((await page.locator('#hud-screen').boundingBox()).width<800);checks++;
  await page.evaluate(()=>{q3rallyFrontend.state(14,0,0,0);q3rallyMapLoading.state('q3r_country01','Country 01','localhost','graphics','',0,0,0);});
  assert.equal(await page.locator('#map-loading-progress').getAttribute('value'),null);
  await page.waitForFunction(()=>!document.querySelector('#map-loading-image').hidden);checks++;
  await page.evaluate(()=>q3rallyMapLoading.state('q3r_country01','Country 01','Server','Loading','track.pk3',50,100,0));
  assert.equal(await page.locator('#map-loading-progress').getAttribute('value'),'50');checks++;
  await page.screenshot({path:path.join(output,'loading.png')});
  await page.setViewportSize({width:390,height:844});
  assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);checks++;
  await page.evaluate(()=>q3rallyFrontend.state(2,0,0,0));
  assert.equal(await page.locator('#map-loading-screen').isHidden(),true);checks++;
  assert.deepEqual(errors,[]);
  fs.writeFileSync(path.join(output,'result.json'),JSON.stringify({success:true,checks,cars:Object.keys(catalog.vehicles).length},null,2));
  console.log('Bundled frontend: '+checks+' checks passed.');
 }catch(error){fs.writeFileSync(path.join(output,'result.json'),JSON.stringify({success:false,checks,error:String(error),errors},null,2));throw error;}
 finally{await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;}).finally(()=>server.close());
