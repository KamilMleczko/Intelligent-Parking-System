(() => {
  // redirect to login if not authenticated
  fetch('/admin').then(r => {
    if (r.status === 401) location.replace('/login');
  });

  const ctx = document.getElementById('entries-chart').getContext('2d');
  const chart = new Chart(ctx, {
    type: 'bar',
    data: { labels: [], datasets: [{ label: 'Entries', data: [] }] },
    options: { responsive: true }
  });

  const ws = new WebSocket(`ws://${location.host}/ws/admin`);
  ws.onmessage = e => {
    const { labels, values } = JSON.parse(e.data);
    chart.data.labels = labels;
    chart.data.datasets[0].data = values;
    chart.update();
  };

  document.getElementById('logout-btn').onclick = async () => {
    await fetch('/logout');
    location.replace('/login');
  };
})();