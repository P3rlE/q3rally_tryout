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
  assert.equal(await page.locator('main:visible').count(),0);checks++;
  await page.evaluate(()=>q3rallyFrontend.state(7,0,0,0));
  assert.equal(await page.locator('#garage').isVisible(),true);
  assert.equal(await page.locator('#frontend').isHidden(),true);checks++;
  await page.evaluate(()=>q3rallyFrontend.state(0,0,0,0));
  assert.equal(await page.locator('main:visible').count(),0);checks++;

  await page.evaluate(()=>document.fonts.ready);
  for(const width of [1920,1280,800,390]){
    await page.setViewportSize({width,height:900});
    const positions=await page.evaluate(()=>{
      const positions=[];
      for(let stage=0;stage<10;stage++){
        q3rallyFrontend.state(1,stage*10,stage%9,stage===9?1:0);
        positions.push([document.querySelector('#loading-stages').getBoundingClientRect().top,
          document.querySelector('#loading-progress').getBoundingClientRect().top]);
      }
      return positions;
    });
    assert.ok(positions.every(position=>position.every((value,index)=>Math.abs(value-positions[0][index])<1)),
      'Loading list and progress must stay fixed at width '+width);checks++;
  }
  await page.setViewportSize({width:1280,height:800});
  const catalog=JSON.parse(fs.readFileSync(path.join(root,'catalog.json')));
  assert.ok(Object.keys(catalog.vehicles).length>=20);
  assert.ok(Object.values(catalog.vehicles).some(car=>car.rank>0));checks++;
  for(const car of Object.values(catalog.vehicles))for(const paint of Object.values(car.paints))assert.ok(fs.existsSync(path.join(root,paint.image)),paint.image);
  await page.evaluate(()=>{q3rallyFrontend.vehicle('alpine','red','svt_cobra');q3rallyFrontend.state(2,0,0,0);});
  await page.waitForFunction(()=>document.querySelector('#active-vehicle-3d canvas'));checks++;
  await page.screenshot({path:path.join(output,'main.png')});
  await page.evaluate(()=>{const c=q3rallyConfig;c.begin(10,'Pause','The online race continues.',0);c.item(0,6,'Resume','',0,0,0,0);c.item(1,6,'Add bots','',0,0,0,0);c.end();q3rallyFrontend.state(12,0,0,0);});
  assert.equal(await page.getByRole('button',{name:'Resume race',exact:true}).count(),1);checks++;
  await page.evaluate(()=>{const c=q3rallyConfig;c.begin(17,'Create your driver','Step 1 / 3.',0);c.item(0,4,'Name','',0,0,31,0);c.item(9,6,'NEXT','',0,0,0,0);c.end();});
  assert.equal(await page.locator('#config-back').isHidden(),true);
  assert.equal(await page.locator('#config-item-0').getAttribute('type'),'text');
  assert.equal(await page.locator('#config-item-0').getAttribute('maxlength'),'31');
  await page.locator('#config-item-0').fill('New.Driver');
  assert.equal(await page.locator('#config-item-0').inputValue(),'New.Driver');checks++;
  await page.screenshot({path:path.join(output,'registration-create.png')});
  await page.evaluate(()=>{const c=q3rallyConfig;c.begin(18,'Make it yours','Optional details.',0);c.item(5,1,'Country','',1,0,0,0);c.choice(5,0,'Not specified');c.choice(5,1,'Germany');c.item(7,6,'Back','',0,0,0,0);c.item(9,6,'NEXT','',0,0,0,0);c.end();});
  assert.equal(await page.locator('#config-item-5').inputValue(),'1');
  assert.equal(await page.locator('#config-back').isVisible(),true);checks++;
  const avatarKeys=Object.keys(catalog.avatars);assert.ok(avatarKeys.length>=2);
  await page.evaluate(keys=>{const c=q3rallyConfig;c.begin(18,'Make it yours','Choose your avatar.',0);c.item(6,1,'Avatar','',1,0,0,0);c.choice(6,0,'No avatar');c.choice(6,1,'Avatar preset',keys[0]);c.choice(6,2,'Avatar preset',keys[1]);c.end();},avatarKeys);
  const avatar=page.locator('.config-avatar-preview img');
  await page.waitForFunction(()=>{const image=document.querySelector('.config-avatar-preview img');return image&&!image.hidden&&image.complete&&image.naturalWidth>0;});
  assert.equal(await avatar.getAttribute('src'),catalog.avatars[avatarKeys[0]]);checks++;
  await page.locator('#config-item-6').evaluate(input=>{input.value='2';input.dispatchEvent(new Event('input'));});
  assert.equal(await avatar.getAttribute('src'),catalog.avatars[avatarKeys[1]]);checks++;
  await page.locator('#config-item-6').evaluate(input=>{input.value='0';input.dispatchEvent(new Event('input'));});
  assert.equal(await avatar.isHidden(),true);
  assert.equal(await page.locator('.config-avatar-preview span').textContent(),'No avatar selected');checks++;
  await page.locator('#config-item-6').evaluate(input=>{input.value='1';input.dispatchEvent(new Event('input'));});
  await page.waitForFunction(()=>document.querySelector('.config-avatar-preview img').naturalWidth>0);
  await page.screenshot({path:path.join(output,'registration-avatar.png')});
  await page.evaluate(()=>{const c=q3rallyConfig;c.begin(19,'Offline Ladder','Connecting to ladder...',0);c.item(7,6,'Back','',0,0,0,1);c.item(8,6,'SKIP','',0,0,0,1);c.item(9,6,'JOIN LADDER','',0,0,0,1);c.end();});
  assert.equal(await page.locator('#config-back').isDisabled(),true);
  assert.equal(await page.getByRole('button',{name:'JOIN LADDER',exact:true}).isDisabled(),true);checks++;
  await page.evaluate(()=>{const c=q3rallyConfig;c.begin(16,'Register offline profile','Registration failed. Try again.',0);c.item(0,4,'Owner','New.Driver',0,0,48,0);c.item(1,4,'Email','',0,0,64,0);c.item(2,6,'RETRY','',0,0,0,0);c.end();});
  assert.equal(await page.locator('#config-item-1').getAttribute('type'),'email');
  assert.equal(await page.locator('#config-description').textContent(),'Registration failed. Try again.');
  assert.equal(await page.getByRole('button',{name:'RETRY',exact:true}).isEnabled(),true);checks++;
  await page.screenshot({path:path.join(output,'registration-offline.png')});
  await page.evaluate(()=>{const c=q3rallyConfig;c.begin(20,'Ready to race','Your driver is ready.',0);c.item(9,6,'START RACING','',0,0,0,0);c.end();});
  assert.equal(await page.locator('#config-back').isHidden(),true);
  assert.equal(await page.getByRole('button',{name:'START RACING',exact:true}).isEnabled(),true);checks++;
  await page.evaluate(()=>{const c=q3rallyConfig;c.begin(21,'Save demo recording','That filename already exists',0);c.item(0,4,'Filename (without extension)','demo001',0,0,40,0);c.item(1,6,'Save','',0,0,0,0);c.item(2,6,'Keep name','',0,0,0,0);c.end();});
  assert.equal(await page.locator('#config-item-0').inputValue(),'demo001');
  assert.equal(await page.getByRole('button',{name:'Save',exact:true}).isEnabled(),true);
  assert.equal(await page.getByRole('button',{name:'Keep name',exact:true}).isEnabled(),true);checks++;
  await page.setViewportSize({width:390,height:844});
  assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),true);checks++;
  await page.setViewportSize({width:1280,height:800});
  await page.evaluate(()=>{const c=q3rallyConfig;c.begin(9,'Confirm change','Delete this profile?',1);c.item(0,6,'Yes','',0,0,0,0);c.item(1,6,'No','',0,0,0,0);c.end();});
  assert.equal(await page.locator('#config-confirm').isVisible(),true);checks++;
  await page.evaluate(()=>{q3rallyConfig.begin(21,'Save demo recording','',0);q3rallyConfig.end();});
  assert.equal(await page.locator('#config-confirm').isVisible(),false);checks++;
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
  // Resolve obsolete model requests last: neither the old photo nor a stale
  // 3D model may be displayed under the new selection's title.
  await page.evaluate(()=>{
    window.__previewRequests=[];window.__previewShown=[];
    window.Q3RallyShowroom={openVehicleShowroom:(host,base,id)=>new Promise(resolve=>{
      const entry={id,resolve:()=>resolve({update:async()=>{},show:()=>__previewShown.push(id),dispose:()=>{},reset:()=>{}})};
      __previewRequests.push(entry);
    })};
    q3rallyFrontend.state(7,0,0,0);
    document.querySelector('#view-toggle').click();
    const field=document.querySelector('#vehicle');field.selectedIndex=1;field.dispatchEvent(new Event('change'));
    field.selectedIndex=2;field.dispatchEvent(new Event('change'));
  });
  assert.equal(await page.locator('#vehicle-image').isHidden(),true);
  assert.equal(await page.locator('#vehicle-3d').evaluate(e=>getComputedStyle(e).visibility),'hidden');checks++;
  const latest=await page.locator('#vehicle').inputValue();
  await page.evaluate(()=>{[...__previewRequests].reverse().forEach(request=>request.resolve());});
  await page.waitForFunction(()=>__previewShown.length>0);
  assert.deepEqual(await page.evaluate(()=>__previewShown),[latest]);checks++;
  await page.evaluate(()=>{
    document.querySelector('#view-toggle').click();
    __previewRequests=[];__previewShown=[];
    const l=q3rallyLibrary;l.begin(5);
    l.item(5,0,['First','alpine/red','','','Plasmagun'],0,0);
    l.item(5,1,['Second','alpine/blue','','','Railgun'],0,0);
    l.end(5);q3rallyFrontend.state(5,0,0,0);
    document.querySelector('#library-list button[data-index="1"]').click();
  });
  assert.equal(await page.locator('#library-image').isHidden(),true);
  assert.equal(await page.locator('#rival-3d').evaluate(e=>getComputedStyle(e).visibility),'hidden');checks++;
  await page.evaluate(()=>{[...__previewRequests].reverse().forEach(request=>request.resolve());});
  await page.waitForFunction(()=>__previewShown.length>0);
  assert.equal(await page.evaluate(()=>__previewShown.length),1);
  assert.equal(await page.locator('#library-name').textContent(),'Second');checks++;
  assert.equal(await page.locator('#library-weapon span').textContent(),'Favorite weapon / Railgun');
  assert.equal(await page.locator('#library-weapon img').getAttribute('src'),catalog.weaponIcons['icons/iconw_railgun']);
  await page.waitForFunction(()=>{const icon=document.querySelector('#library-weapon img');return icon.complete&&icon.naturalWidth>0;});checks++;

  assert.deepEqual(errors,[]);
  fs.writeFileSync(path.join(output,'result.json'),JSON.stringify({success:true,checks,cars:Object.keys(catalog.vehicles).length},null,2));
  console.log('Bundled frontend: '+checks+' checks passed.');
 }catch(error){fs.writeFileSync(path.join(output,'result.json'),JSON.stringify({success:false,checks,error:String(error),errors},null,2));throw error;}
 finally{await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;}).finally(()=>server.close());
