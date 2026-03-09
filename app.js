const API_BASE = "http://localhost:8080/cgi-bin";


// API calls — each maps to one CGI endpoint

async function apiGetListings() {
    if (API_BASE) {
        const r = await fetch(`${API_BASE}/get_listings.cgi`);
        return r.json();
    }
    return market.active_listings;
}

async function apiPlaceBid(item_name, userName, bidAmount) {
    if (API_BASE) {
        const r = await fetch(`${API_BASE}/place_bid.cgi`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `item_name=${encodeURIComponent(item_name)}&userName=${encodeURIComponent(userName)}&bidAmount=${bidAmount}`
        });
        return r.json();
    }
    return demoAddBid(item_name, userName, bidAmount);
}

async function apiListItem(item_name, sPrice, binPrice, expiration, username) {
    if (API_BASE) {
        const r = await fetch(`${API_BASE}/list_item.cgi`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ item_name, sPrice, binPrice, expiration, username })
        });
        return r.json();
    }
    return demoListItem(item_name, sPrice, binPrice, expiration, username);
}

async function apiAddUser(username) {
    if (API_BASE) {
        const r = await fetch(`${API_BASE}/add_user.cgi`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `username=${encodeURIComponent(username)}`
        });
        return r.json();
    }
    return { success: true };
}


// Data — mirrors eBay.h
// Bid    { user, amount }
// Items  { item_name, winner, win, sPrice, binPrice, expiration, highestBid, bids[] }
// User   { username, isSeller, history[30], selling, interest[] }
// Market { active_listings[], users{}, logs[] }

const market = {
    active_listings: [],
    users:           {},
    logs:            [],
};

for (let i = 0; i < 10; i++) {
    const username = `user_${i}`;
    market.users[username] = {
        username,
        isSeller: false,
        history:  new Array(30).fill(null),
        selling:  null,
        interest: [],
    };
}

function nowSecs()       { return Math.floor(Date.now() / 1000); }
function expiryTs(hours) { return nowSecs() + Math.round(hours * 3600); }


// Demo logic — mirrors C++ method bodies

// Items::addBid(const std::string userName, const double bidAmount)
function demoAddBid(item_name, userName, bidAmount) {
    const item = market.active_listings.find(i => i.item_name === item_name);
    if (!item) return { success: false, message: 'Item not found.' };

    if (item.win) return { success: false, message: 'Item already won.' };

    if (item.binPrice > 0 && bidAmount >= item.binPrice) {
        item.win        = true;
        item.winner     = userName;
        item.highestBid = item.binPrice;
        item.bids.push({ user: userName, amount: item.binPrice });

        const seller = market.users[item.sellerUsername];
        if (seller) seller.selling = null;

        market.logs.push(`${userName} bought "${item_name}" for $${item.binPrice.toFixed(2)}`);
        demoAddHistory(userName, item);

        return { success: true, message: `Bought for $${item.binPrice.toFixed(2)}!`, won: true };
    }
    else if (bidAmount >= item.highestBid) {
        item.highestBid = bidAmount;
        item.bids.push({ user: userName, amount: bidAmount });

        market.logs.push(`${userName} bid $${bidAmount.toFixed(2)} on "${item_name}"`);
        demoAddHistory(userName, item);

        return { success: true, message: 'Bid placed!' };
    }
    else {
        return { success: false, message: `Bid must be at least $${item.highestBid.toFixed(2)}.` };
    }
}

// Market::addItem() + Items setters: setItem(), setPrices(), setTime()
function demoListItem(item_name, sPrice, binPrice, expiration, username) {
    const user = market.users[username];
    if (!user) return { success: false, message: 'User not found.' };
    if (user.selling) return { success: false, message: `Already listing "${user.selling.item_name}". One item at a time.` };

    user.isSeller = true;

    const item = {
        item_name,
        winner:         '',
        win:            false,
        sPrice,
        binPrice:       binPrice || 0.0,
        expiration:     expiryTs(expiration),
        highestBid:     sPrice,
        bids:           [],
        sellerUsername: username,
    };

    market.active_listings.push(item);
    user.selling = item;
    market.logs.push(`${username} listed "${item_name}" starting at $${sPrice.toFixed(2)}`);

    return { success: true, message: 'Item listed!' };
}

// User::addHistory(const Items& add)
function demoAddHistory(username, item) {
    const user = market.users[username];
    if (!user) return;

    for (let i = 0; i < 30; i++) {
        if (user.history[i] === null) {
            user.history[i] = item;
            return;
        }
    }

    for (let i = 0; i < 29; i++) {
        user.history[i] = user.history[i + 1];
    }
    user.history[29] = item;
}

// User::interested(const std::string& name)
function demoInterested(username, item_name) {
    const user = market.users[username];
    if (!user) return false;
    let i = 0;
    while (i < user.interest.length) {
        if (user.interest[i] !== null && user.interest[i].item_name === item_name)
            return true;
        i++;
    }
    return false;
}

// User::removeInterest(Items* item)
function demoRemoveInterest(username, item_name) {
    const user = market.users[username];
    if (!user) return;
    let i = 0;
    while (i < user.interest.length) {
        if (user.interest[i] && user.interest[i].item_name === item_name) {
            user.interest.splice(i, 1);
            return;
        }
        i++;
    }
}


// Helpers

function currentUser() {
    return market.users[`user_${document.getElementById('currentUser').value}`];
}

function fmt(n) {
    return '$' + Number(n).toLocaleString('en-US', {
        minimumFractionDigits: 2, maximumFractionDigits: 2
    });
}

function timeLeft(expiration) {
    const ms = expiration * 1000 - Date.now();
    if (ms <= 0) return { str: 'Ended', urgent: true, ended: true };
    const h = Math.floor(ms / 3600000);
    const m = Math.floor((ms % 3600000) / 60000);
    const s = Math.floor((ms % 60000) / 1000);
    if (h > 0) return { str: `${h}h ${m}m`, urgent: false, ended: false };
    if (m > 0) return { str: `${m}m ${s}s`, urgent: true,  ended: false };
    return     { str: `${s}s`,              urgent: true,  ended: false };
}


// View switching

let currentView   = 'listings';
let activeBidItem = null;

function showView(view, btn) {
    document.querySelectorAll('.nav-links button').forEach(b => b.classList.remove('active'));
    if (btn) btn.classList.add('active');
    ['listings', 'watchlist', 'history'].forEach(v => {
        document.getElementById(`view-${v}`).style.display = v === view ? '' : 'none';
    });
    currentView = view;
    if (view === 'watchlist') renderWatchlist();
    if (view === 'history')   renderHistory();
}


function onUserChange() {
    renderListings();
    if (currentView === 'watchlist') renderWatchlist();
    if (currentView === 'history')   renderHistory();
}


// Render listings

async function renderListings() {
    const all    = await apiGetListings();

    // Keep local cache in sync so openBidModal can find items
    if (API_BASE) market.active_listings = all;

    const search = document.getElementById('searchInput').value.toLowerCase();
    const sort   = document.getElementById('sortSelect').value;

    let filtered = all.filter(item =>
        !search || item.item_name.toLowerCase().includes(search)
    );

    filtered.sort((a, b) => {
        if (sort === 'expiry')   return a.expiration - b.expiration;
        if (sort === 'bid-high') return b.highestBid - a.highestBid;
        if (sort === 'bid-low')  return a.highestBid - b.highestBid;
        return 0;
    });

    document.getElementById('stat-active').textContent =
        all.filter(x => !x.win && !timeLeft(x.expiration).ended).length;
    document.getElementById('stat-bids').textContent =
        all.reduce((sum, x) => sum + (x.bids ? x.bids.length : 0), 0);

    // Check if any expired item was won by the current user and add to history
    const user = currentUser();
    all.forEach(item => {
        if (item.win && item.winner === user.username) {
            const alreadyInHistory = user.history.some(h => h && h.item_name === item.item_name);
            if (!alreadyInHistory) user.history.push({ ...item, userBid: item.highestBid });
        }
    });

    const grid = document.getElementById('listingsGrid');

    if (filtered.length === 0) {
        grid.innerHTML = `
            <div class="empty" style="grid-column:1/-1">
                <p>No listings yet. Be the first to list an item!</p>
            </div>`;
        return;
    }

    grid.innerHTML = filtered.map((item, idx) => {
        const tl      = timeLeft(item.expiration);
        const ended   = tl.ended || item.win;
        const watched = demoInterested(user.username, item.item_name);

        const badge = ended ? '' : tl.urgent
            ? `<span class="badge badge-closing">Closing Soon</span>`
            : `<span class="badge badge-active">Active</span>`;

        return `
        <div class="card" style="animation-delay:${idx * 25}ms"
          onclick="openBidModal('${item.item_name}')">
          <div class="card-header">
            <div class="card-title">${item.item_name}</div>
            <button class="card-watch ${watched ? 'watching' : ''}"
              onclick="event.stopPropagation(); toggleWatch('${item.item_name}', this)">
              ${watched ? '★' : '☆'}
            </button>
          </div>
          <div class="card-seller">Seller: ${item.sellerUsername}</div>
          ${badge}
          <div class="card-bid-label">${ended ? 'FINAL BID' : 'CURRENT BID'}</div>
          <div class="card-bid-amount">${fmt(item.highestBid)}</div>
          <div class="card-bid-label" style="margin-top:4px">STARTING PRICE</div>
          <div class="card-start">${fmt(item.sPrice)}</div>
          ${item.binPrice > 0
            ? `<div class="card-bin">Buy It Now: <span>${fmt(item.binPrice)}</span></div>`
            : `<div class="card-bin">&nbsp;</div>`}
          <div class="timer ${tl.urgent ? 'urgent' : ''}">⏱ ${tl.str}</div>
          ${ended
            ? `<div class="card-status">
                ${item.win ? `SOLD — ${item.winner}` : 'AUCTION ENDED'}
               </div>`
            : `<div class="card-actions">
                <button class="btn-primary"
                  onclick="event.stopPropagation(); openBidModal('${item.item_name}')">Bid</button>
                ${item.binPrice > 0
                  ? `<button class="btn-secondary"
                      onclick="event.stopPropagation(); doBuyNow('${item.item_name}')">Buy Now</button>`
                  : ''}
               </div>`
          }
        </div>`;
    }).join('');
}


// Bid modal

function openBidModal(item_name) {
    const item = market.active_listings.find(i => i.item_name === item_name);
    if (!item) return;
    activeBidItem = item;
    const tl = timeLeft(item.expiration);

    document.getElementById('bm-title').textContent  = item.item_name;
    document.getElementById('bm-meta').textContent   = `Seller: ${item.sellerUsername}`;
    document.getElementById('bm-curbid').textContent = fmt(item.highestBid);
    document.getElementById('bm-bin').textContent    = item.binPrice > 0 ? fmt(item.binPrice) : '—';
    document.getElementById('bm-time').textContent   = tl.str;

    const minBid = (item.highestBid + 0.01).toFixed(2);
    document.getElementById('bm-min').textContent    = '$' + minBid;
    document.getElementById('bidAmount').value       = '';
    document.getElementById('bidAmount').placeholder = minBid;
    document.getElementById('bidMsg').textContent    = '';
    document.getElementById('bidMsg').className      = 'msg';

    const recent = [...item.bids].reverse().slice(0, 5);
    document.getElementById('bidHistoryList').innerHTML = recent.length === 0
        ? `<div style="font-size:13px;color:var(--muted);padding:8px 0">No bids yet.</div>`
        : recent.map((b, i) => `
            <div class="bid-row ${i === 0 ? 'top' : ''}">
              <span class="bidder">${b.user}</span>
              <span class="bid-amt">${fmt(b.amount)}</span>
            </div>`).join('');

    document.getElementById('bidModal').classList.add('open');
}

function closeBidModal() {
    document.getElementById('bidModal').classList.remove('open');
    activeBidItem = null;
}

async function submitBid() {
    if (!activeBidItem) return;
    const bidAmount = parseFloat(document.getElementById('bidAmount').value);
    const msg       = document.getElementById('bidMsg');

    if (isNaN(bidAmount) || bidAmount <= 0) {
        msg.textContent = 'Enter a valid amount.';
        msg.className   = 'msg';
        return;
    }

    const result = await apiPlaceBid(activeBidItem.item_name, currentUser().username, bidAmount);

    if (result.success) {
        const user = currentUser();
        const alreadyInHistory = user.history.some(h => h && h.item_name === activeBidItem.item_name);
        if (!alreadyInHistory) user.history.push({ ...activeBidItem, userBid: bidAmount });

        msg.textContent = '✓ ' + result.message;
        msg.className   = 'msg success';
        setTimeout(() => { closeBidModal(); renderListings(); }, 800);
    } else {
        msg.textContent = result.message;
        msg.className   = 'msg';
    }
}

async function doBuyNow(item_name) {
    const item = market.active_listings.find(i => i.item_name === item_name);
    if (!item) return;
    const result = await apiPlaceBid(item_name, currentUser().username, item.binPrice);
    if (result.success) {
        const user = currentUser();
        const alreadyInHistory = user.history.some(h => h && h.item_name === item_name);
        if (!alreadyInHistory) user.history.push({ ...item, userBid: item.binPrice });

        renderListings();
        alert(result.message);
    } else {
        alert(result.message);
    }
}


// Watchlist — User::interest[]

function toggleWatch(item_name, btn) {
    const user = currentUser();
    if (demoInterested(user.username, item_name)) {
        demoRemoveInterest(user.username, item_name);
        btn.textContent = '☆';
        btn.classList.remove('watching');
    } else {
        const item = market.active_listings.find(i => i.item_name === item_name);
        if (item) user.interest.push(item);
        btn.textContent = '★';
        btn.classList.add('watching');
    }
}

function renderWatchlist() {
    const el      = document.getElementById('watchlistContent');
    const user    = currentUser();
    const watched = user.interest.filter(i => i !== null);

    if (watched.length === 0) {
        el.innerHTML = `
            <div class="empty">
                <p>Nothing in your watchlist yet.</p>
                <p>Click the star on any listing to track it.</p>
            </div>`;
        return;
    }

    el.innerHTML = watched.map(item => {
        const tl = timeLeft(item.expiration);
        return `
        <div class="list-row"
          onclick="showView('listings', document.querySelector('.nav-links button'));
                   openBidModal('${item.item_name}')">
          <div class="list-row-info">
            <div class="row-title">${item.item_name}</div>
            <div class="row-meta">Seller: ${item.sellerUsername} · ${tl.str}</div>
          </div>
          <div class="list-row-price">${fmt(item.highestBid)}</div>
          <button class="btn-remove"
            onclick="event.stopPropagation();
                     demoRemoveInterest('${user.username}', '${item.item_name}');
                     renderWatchlist()">x</button>
        </div>`;
    }).join('');
}


// Bid history — User::history[30]

function renderHistory() {
    const el      = document.getElementById('historyContent');
    const user    = currentUser();
    const entries = user.history.filter(h => h !== null);

    if (entries.length === 0) {
        el.innerHTML = `<div class="empty"><p>No bid history yet.</p></div>`;
        return;
    }

    el.innerHTML = [...entries].reverse().map(item => `
        <div class="list-row">
          <span class="status-badge ${item.win && item.winner === user.username ? 'status-won' : 'status-active'}">
            ${item.win && item.winner === user.username ? 'Bought' : 'Bid'}
          </span>
          <div class="list-row-info">
            <div class="row-title">${item.item_name}</div>
            <div class="row-meta">${item.win && item.winner === user.username ? 'Bought for' : 'Your bid'}: ${fmt(item.userBid || item.highestBid)}</div>
          </div>
        </div>`).join('');
}


// New listing modal

function openNewListing() {
    const user = currentUser();
    if (user.selling) {
        alert(`Already listing "${user.selling.item_name}". One item at a time.`);
        return;
    }
    document.getElementById('nlMsg').textContent = '';
    document.getElementById('newListingModal').classList.add('open');
}

function closeNewListing() {
    document.getElementById('newListingModal').classList.remove('open');
}

async function submitListing() {
    const item_name  = document.getElementById('nl-title').value.trim();
    const sPrice     = parseFloat(document.getElementById('nl-start').value);
    const binPrice   = parseFloat(document.getElementById('nl-bin').value) || 0.0;
    const expiration = parseFloat(document.getElementById('nl-dur').value);
    const msg        = document.getElementById('nlMsg');

    if (!item_name)             { msg.textContent = 'Enter an item name.'; return; }
    if (!sPrice || sPrice <= 0) { msg.textContent = 'Enter a valid starting price.'; return; }
    if (binPrice > 0 && binPrice <= sPrice) {
        msg.textContent = 'Buy It Now price must exceed starting price.';
        return;
    }

    const result = await apiListItem(item_name, sPrice, binPrice, expiration, currentUser().username);

    if (result.success) {
        msg.textContent = '';
        closeNewListing();
        document.getElementById('nl-title').value = '';
        document.getElementById('nl-start').value = '';
        document.getElementById('nl-bin').value   = '';
        renderListings();
    } else {
        msg.textContent = result.message || 'Failed to list item.';
    }
}


// Close modals on overlay click

document.getElementById('bidModal').addEventListener('click', function(e) {
    if (e.target === this) closeBidModal();
});
document.getElementById('newListingModal').addEventListener('click', function(e) {
    if (e.target === this) closeNewListing();
});


// Init

// Register all 10 users with the backend on page load
async function initUsers() {
    for (let i = 0; i < 10; i++) {
        await apiAddUser(`user_${i}`);
    }
}

initUsers();
renderListings();
setInterval(() => { if (currentView === 'listings') renderListings(); }, 5000);
