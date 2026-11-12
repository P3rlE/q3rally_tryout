// Exercise the real CEF/native menu bridge, rather than setting video CVars directly.
const fs=require('node:fs');
const path=require('node:path');
const port=Number(process.argv[2]),output=path.resolve(process.argv[3]);
const pause=()=>new Promise(resolve=>setTimeout(resolve,100));
async function target(previous){
 const deadline=Date.now()+45000;
 while(Date.now()<deadline){
  try{const list=await(await fetch('http://127.0.0.1:'+port+'/json/list',{signal:AbortSignal.timeout(2000)})).json();const page=list.find(p=>p.type==='page'&&p.url==='https://q3rally.invalid/index.html'&&p.id!==previous);if(page)return page;}catch{}
  await pause();
 }
 throw Error('Timed out waiting for replacement CEF page');
}
async function connect(page){
 const socket=new WebSocket(page.webSocketDebuggerUrl),pending=new Map();let serial=0;
 await new Promise((resolve,reject)=>{socket.addEventListener('open',resolve,{once:true});socket.addEventListener('error',reject,{once:true});});
 socket.addEventListener('message',event=>{const message=JSON.parse(event.data);const request=pending.get(message.id);if(request){pending.delete(message.id);clearTimeout(request.timer);message.error?request.reject(Error(message.error.message)):request.resolve(message.result);}});
 socket.addEventListener('close',()=>{for(const request of pending.values()){clearTimeout(request.timer);request.reject(Error('CEF page closed'));}pending.clear();});
 const call=(method,params={})=>new Promise((resolve,reject)=>{const id=++serial;const timer=setTimeout(()=>{pending.delete(id);reject(Error('CDP timeout: '+method));},10000);pending.set(id,{resolve,reject,timer});socket.send(JSON.stringify({id,method,params}));});
 return {call,close:()=>socket.close(),evaluate:async expression=>{const reply=await call('Runtime.evaluate',{expression,returnByValue:true,userGesture:true});if(reply.exceptionDetails)throw Error(reply.exceptionDetails.text);return reply.result.value;}};
}
async function wait(client,expression){const deadline=Date.now()+30000;while(Date.now()<deadline){if(await client.evaluate(expression))return;await pause();}fs.writeFileSync(path.join(output,'timeout-dom.txt'),await client.evaluate('document.body.innerText'));throw Error('Timed out: '+expression);}
async function click(client,query){
 const expression=`(()=>{const element=${query};return !!element&&!element.disabled&&element.getClientRects().length>0;})()`;
 await wait(client,expression);
 await client.evaluate(`(()=>{const element=${query};element.focus();element.click();})()`);
}
const button=name=>`[...document.querySelectorAll('#config-actions button')].find(b=>b.textContent.trim()===${JSON.stringify(name)})`;
(async()=>{
 let page=await target(),client=await connect(page);
 await click(client,"document.querySelector('#main-navigation [data-action=\"config\"]')");
 // Form rebuilds can leave focus on BODY. ESC must still leave the menu,
 // once per physical press, without destroying/reloading the CEF page.
 await wait(client,button('Graphics'));
 await client.evaluate("document.activeElement.blur()");
 await client.call('Input.dispatchKeyEvent',{type:'rawKeyDown',key:'Escape',code:'Escape',windowsVirtualKeyCode:27});
 await client.call('Input.dispatchKeyEvent',{type:'keyUp',key:'Escape',code:'Escape',windowsVirtualKeyCode:27});
 await wait(client,"!document.querySelector('#main-navigation').hidden");
 const current=await(await fetch('http://127.0.0.1:'+port+'/json/list')).json();
 if(!current.some(item=>item.id===page.id))throw Error('ESC unexpectedly recreated CEF');
 console.log('Single ESC from BODY returned to main menu.');
 await click(client,"document.querySelector('#main-navigation [data-action=\"config\"]')");
 let checks=0;
 for(const value of ['0','1','0']){
  await click(client,button('Graphics'));
  const fullscreen="[...document.querySelectorAll('#config-fields label')].find(e=>e.textContent.trim()==='Fullscreen')";
  await wait(client,fullscreen);
  await client.evaluate("(()=>{window.__nativeConfigUpdates=0;const end=q3rallyConfig.end;q3rallyConfig.end=function(...args){end(...args);window.__nativeConfigUpdates++;};})()");
  await client.evaluate(`(()=>{const label=${fullscreen},input=document.getElementById(label.htmlFor);input.value=${JSON.stringify(value)};input.dispatchEvent(new Event('change',{bubbles:true}));})()`);
  await wait(client,'window.__nativeConfigUpdates>0');
  console.log('Fullscreen setting acknowledged: '+value);
  await click(client,button('Apply'));
  const previous=page.id;client.close();page=await target(previous);client=await connect(page);
  await wait(client,button('Graphics'));
  checks++;
  console.log('Apply restart completed; fullscreen='+value);
 }
 const shot=await client.call('Page.captureScreenshot');fs.writeFileSync(path.join(output,'graphics-apply.png'),Buffer.from(shot.data,'base64'));
 await click(client,"document.querySelector('#config-back')");
 await click(client,"document.querySelector('#main-navigation [data-action=\"quit\"]')");
 await click(client,"document.querySelector('#quit-accept')");
 await click(client,"document.querySelector('#credits-exit')");
 fs.writeFileSync(path.join(output,'menu-result.json'),JSON.stringify({success:true,checks},null,2));
 console.log('Native Graphics / Fullscreen / Apply: '+checks+' restarts passed.');
 process.exit(0);
})().catch(error=>{fs.writeFileSync(path.join(output,'menu-result.json'),JSON.stringify({success:false,error:String(error)},null,2));console.error(error);process.exit(1);});
