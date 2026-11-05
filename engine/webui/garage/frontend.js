'use strict';



(() => {

  const shell = document.querySelector('#frontend');

  const garage = document.querySelector('#garage');

  const navigation = document.querySelector('#main-navigation');

  const loading = document.querySelector('#loading-panel');

  const labels = ['Starting up', 'Loading your car', 'Preparing the setup menu', 'Opening the garage',

    'Driver settings', 'Reading your controls', 'Server browser', 'Loading the track list', 'Ready to race'];

  const rows = labels.slice(0, 8).map(label => {

    const row = document.createElement('li');

    row.textContent = label;

    document.querySelector('#loading-stages').append(row);

    return row;

  });

  let activeSetup;

  let viewer;

  let viewerKey;

  let request = 0;



  async function preview() {

    if (shell.hidden || !activeSetup || !window.Q3RallyShowroom) return;

    const setup = activeSetup;

    const key = [setup.id, setup.paint, setup.wheels].join('/');

    if (key === viewerKey) {

      viewer?.show();

      return;

    }

    viewerKey = key;

    const version = ++request;

    viewer?.dispose();

    viewer = undefined;

    try {

      const response = await fetch('./catalog.json');

      if (!response.ok) throw new Error('Catalog unavailable');

      const catalog = await response.json();

      const car = catalog.vehicles[setup.id];

      if (!car) throw new Error('Vehicle unavailable');

      document.querySelector('#active-vehicle-name').textContent = car.name;

      const next = await window.Q3RallyShowroom.openVehicleShowroom(

        document.querySelector('#active-vehicle-3d'), './showroom/', setup.id, car.name,
        { yawDegrees: 135, distance: 2.6 });

      if (version !== request) { next.dispose(); return; }

      viewer = next;

      await next.update(setup.paint, setup.wheels);

      if (version === request) next.show();

    } catch {

      if (version !== request) return;

      viewer?.dispose();

      viewer = undefined;

      viewerKey = undefined;

      document.querySelector('#active-vehicle-status').textContent = 'Open the garage to choose your setup.';

    }

  }



  window.q3rallyFrontend = {

    state(screen, percent, stage, prompt) {
      window.q3rallyArchive?.show(screen);
      window.q3rallyLibrary?.show(screen);
      window.q3rallyDriver?.show(screen);
      window.q3rallyOffline?.show(screen);
      window.q3rallyOnline?.show(screen);
      window.q3rallyConfig?.show(screen);
      window.q3rallyHUD?.show(screen);
      window.q3rallyMapLoading?.show(screen);
      if ((screen >= 3 && screen <= 6) || screen === 8 || screen === 9 || screen === 10 || screen === 11 || screen === 12 || screen === 13 || screen === 14) {
        shell.hidden = garage.hidden = true;
        request++;
        viewer?.dispose();
        viewer = undefined;
        viewerKey = undefined;
        return;
      }

      const entering = shell.hidden;

      shell.hidden = false;

      garage.hidden = true;

      navigation.hidden = screen !== 2;

      loading.hidden = screen !== 1;

      document.querySelector('#open-garage').hidden = screen !== 2;

      document.querySelector('#frontend-title').innerHTML = screen === 1

        ? 'Getting <em>ready.</em>' : 'Ready to <em>race.</em>';

      document.querySelector('#frontend-status').textContent = screen === 1 ? 'Loading graphics' : 'Main menu';

      document.querySelector('#loading-title').textContent = prompt ? 'Update available' : labels[stage] ?? labels[0];

      document.querySelector('#loading-progress').value = percent;

      document.querySelector('#loading-percent').textContent = percent + '%';

      document.querySelector('#update-prompt').hidden = !prompt;

      rows.forEach((row, index) => {

        row.classList.toggle('complete', index < stage);

        row.classList.toggle('current', index === stage);

        row.setAttribute('aria-current', index === stage ? 'step' : 'false');

      });

      if (entering) void preview();

    },

    async profile(name, rank, points) {

      const summary = document.querySelector('#profile-summary');

      summary.textContent = name || 'Create your driver profile';

      try {

        const response = await fetch('./catalog.json');

        const catalog = await response.json();

        summary.textContent = (name || 'Driver') + ' \u00b7 ' + (catalog.ranks[rank] ?? '') + ' \u00b7 ' + points + ' points';

      } catch { /* The profile button remains available when catalog loading fails. */ }

    },

    vehicle(id, paint, wheels) {

      activeSetup = {id, paint, wheels};

      void preview();

    },

    garage() {
      window.q3rallyDriver?.show(7);
      window.q3rallyOffline?.show(7);
      window.q3rallyOnline?.show(7);
      window.q3rallyConfig?.show(7);
      window.q3rallyHUD?.show(7);
      window.q3rallyMapLoading?.show(7);

      shell.hidden = true;

      garage.hidden = false;

      request++;

      viewer?.dispose();

      viewer = undefined;

      viewerKey = undefined;

      document.querySelector('#back').focus();

    },

    leaveGarage() {

      garage.hidden = true;

      shell.hidden = false;

      void preview();

    },

  };

  const quitDialog = document.querySelector('#quit-confirm');
  document.querySelector('#quit-cancel').addEventListener('click', () => quitDialog.close());
  document.querySelector('#quit-accept').addEventListener('click', () => {
    quitDialog.close();
    location.href = 'q3rally-ui://menu/credits-start';
  });
  for (const button of shell.querySelectorAll('[data-action]')) {

    button.addEventListener('click', () => {

      if (button.dataset.action === 'quit') quitDialog.showModal();
      else location.href = 'q3rally-ui://menu/' + button.dataset.action;

    });

  }

})();
