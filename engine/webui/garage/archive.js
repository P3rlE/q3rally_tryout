'use strict';

(() => {
  const creditsScreen = document.querySelector('#credits-screen');
  const demosScreen = document.querySelector('#demos-screen');
  const roll = document.querySelector('#credits-roll');
  const viewport = document.querySelector('#demo-viewport');
  const list = document.querySelector('#demo-list');
  const play = document.querySelector('#demo-play');
  const details = document.querySelector('#demo-details');
  const placeholder = document.querySelector('#demo-placeholder');
  const transport = [...document.querySelectorAll('[data-demo-action]')];
  const recordings = [];
  let animation;
  let playing = false;
  let paused = false;
  let seeking = false;
  let viewportFrame;

  function command(action) {
    if (action !== 0 && !playing) return;
    if (list.selectedIndex < 0) return;
    location.href = 'q3rally-ui://demo/action/' + action + '/' + list.selectedIndex;
  }

  function publishViewport() {
    cancelAnimationFrame(viewportFrame);
    viewportFrame = requestAnimationFrame(() => {
      if (demosScreen.hidden) return;
      const rect = viewport.getBoundingClientRect();
      const x = Math.max(0, Math.round(rect.left));
      const y = Math.max(0, Math.round(rect.top));
      const width = Math.min(Math.round(rect.width), innerWidth - x);
      const height = Math.min(Math.round(rect.height), innerHeight - y);
      if (width <= 0 || height <= 0) return;
      location.href = 'q3rally-ui://demo/viewport/' + [x, y, width, height, innerWidth, innerHeight].join('/');
    });
  }

  function selectRecording() {
    const selected = recordings[list.selectedIndex];
    details.textContent = selected
      ? selected.name + ' / ' + (Math.max(0, selected.size) / 1048576).toFixed(1) + ' MB'
      : 'No recordings found. Record a demo during a game to add it here.';
    play.disabled = !selected;
  }

  function exitCredits() {
    animation?.cancel();
    location.href = 'q3rally-ui://menu/credits-exit';
  }

  window.q3rallyArchive = {
    show(screen) {
      const entering = demosScreen.hidden && screen === 4;
      creditsScreen.hidden = screen !== 3;
      demosScreen.hidden = screen !== 4;
      if (screen !== 3) animation?.cancel();
      if (entering) publishViewport();
    },
    creditsBegin() {
      animation?.cancel();
      roll.replaceChildren();
    },
    demosBegin() {
      recordings.length = 0;
      list.replaceChildren();
    },
    item(screen, index, text, size, kind) {
      if (screen === 3) {
        const line = document.createElement(kind === 2 ? 'h2' : 'p');
        line.textContent = text || '\u00a0';
        line.classList.toggle('label', kind === 1);
        roll.append(line);
      } else {
        recordings[index] = {name: text, size};
        list.add(new Option(text, String(index)));
      }
    },
    creditsEnd() {
      requestAnimationFrame(() => {
        const height = document.querySelector('#credits-window').clientHeight;
        const distance = height + roll.scrollHeight;
        animation = roll.animate(
          [{transform: 'translateY(' + height + 'px)'}, {transform: 'translateY(-' + roll.scrollHeight + 'px)'}],
          {duration: distance / 38 * 1000, easing: 'linear', fill: 'forwards'});
        animation.onfinish = exitCredits;
      });
    },
    demosEnd() {
      if (recordings.length) list.selectedIndex = 0;
      selectRecording();
      transport.forEach(button => { button.disabled = !playing; });
      publishViewport();
    },
    playback(active, frozen, search, time, message) {
      playing = !!active;
      paused = !!frozen;
      seeking = !!search;
      placeholder.hidden = playing && !seeking;
      placeholder.textContent = seeking ? 'Seeking...' : message || 'Select a recording to preview';
      transport.forEach(button => { button.disabled = !playing || seeking; });
      document.querySelector('#demo-pause').textContent = paused ? 'Resume' : 'Pause';
      const seconds = Math.max(0, time);
      document.querySelector('#demo-clock').textContent = Math.floor(seconds / 60) + ':' + String(seconds % 60).padStart(2, '0');
      document.querySelector('#demo-message').textContent = message || (seeking ? 'Seeking...' : playing ? paused ? 'Paused' : 'Playing' : '');
    },
  };

  play.addEventListener('click', () => command(0));
  list.addEventListener('change', selectRecording);
  list.addEventListener('dblclick', () => command(0));
  transport.forEach(button => button.addEventListener('click', () => command(Number(button.dataset.demoAction))));
  document.querySelector('#demos-back').addEventListener('click', () => {
    location.href = 'q3rally-ui://demo/back';
  });
  document.querySelector('#credits-exit').addEventListener('click', exitCredits);
  document.addEventListener('keydown', event => {
    if (!creditsScreen.hidden && !event.repeat) {
      event.preventDefault();
      exitCredits();
    } else if (!demosScreen.hidden && !event.repeat && event.target !== list) {
      if (event.key === 'Escape') document.querySelector('#demos-back').click();
      if (event.key === ' ') { event.preventDefault(); command(1); }
      if (event.key === 'ArrowLeft') command(2);
      if (event.key === 'ArrowRight') command(3);
    }
  });
  new ResizeObserver(publishViewport).observe(viewport);
  addEventListener('resize', publishViewport);
  addEventListener('scroll', publishViewport, {passive: true});
})();
