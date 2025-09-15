// === CampusConnect Shared Utilities ===

// Toast notification system
const Toast = {
  container: null,
  init() {
    if (!this.container) {
      this.container = document.createElement('div');
      this.container.className = 'toast-container';
      document.body.appendChild(this.container);
    }
  },
  show(message, type = 'info', duration = 3500) {
    this.init();
    const icons = { success: '✓', error: '✕', info: 'ℹ' };
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    toast.innerHTML = `<span>${icons[type] || icons.info}</span><span>${message}</span>`;
    this.container.appendChild(toast);
    setTimeout(() => {
      toast.style.animation = 'toastOut 0.3s ease forwards';
      setTimeout(() => toast.remove(), 300);
    }, duration);
  }
};

// Theme toggle
const Theme = {
  init() {
    const saved = localStorage.getItem('cc-theme') || 'dark';
    if (saved === 'light') document.body.classList.add('light-mode');
    document.querySelectorAll('#theme-toggle').forEach(btn => {
      this.updateIcon(btn);
      btn.addEventListener('click', () => this.toggle());
    });
  },
  toggle() {
    document.body.classList.toggle('light-mode');
    const isLight = document.body.classList.contains('light-mode');
    localStorage.setItem('cc-theme', isLight ? 'light' : 'dark');
    document.querySelectorAll('#theme-toggle').forEach(btn => this.updateIcon(btn));
  },
  updateIcon(btn) {
    const isLight = document.body.classList.contains('light-mode');
    btn.textContent = isLight ? '🌙' : '☀️';
    btn.title = isLight ? 'Switch to dark mode' : 'Switch to light mode';
  }
};

// Auth helpers
const Auth = {
  getUser() {
    return {
      id: localStorage.getItem('CampusConnectUserId') || '',
      name: localStorage.getItem('CampusConnectUserName') || 'User',
      role: localStorage.getItem('CampusConnectUserRole') || 'student'
    };
  },
  logout() {
    localStorage.removeItem('CampusConnectUserId');
    localStorage.removeItem('CampusConnectUserName');
    localStorage.removeItem('CampusConnectUserRole');
    window.location.href = '/index.html';
  },
  requireAuth() {
    if (!localStorage.getItem('CampusConnectUserId')) {
      window.location.href = '/index.html';
      return false;
    }
    return true;
  },
  initNavbar() {
    const user = this.getUser();
    const nameEl = document.getElementById('navbar-user-name');
    const roleEl = document.getElementById('navbar-user-role');
    if (nameEl) nameEl.textContent = user.name;
    if (roleEl) {
      roleEl.textContent = user.role;
      roleEl.className = 'user-badge';
    }
    const logoutBtn = document.getElementById('logout-btn');
    if (logoutBtn) logoutBtn.addEventListener('click', (e) => { e.preventDefault(); Auth.logout(); });
  }
};

document.addEventListener('DOMContentLoaded', () => Theme.init());
