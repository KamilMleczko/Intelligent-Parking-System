const form = document.getElementById('login-form');
const errorEl = document.getElementById('error');

form.addEventListener('submit', async e => {
  e.preventDefault();
  const resp = await fetch('/login', { method: 'POST', body: new FormData(form) });
  if (resp.redirected) {
    window.location = resp.url;
  } else {
    errorEl.textContent = 'Invalid credentials';
    errorEl.classList.remove('hidden');
  }
});