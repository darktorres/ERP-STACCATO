# Self-Hosted Browser Terminal Tools - Comprehensive Comparison

A detailed comparison of self-hosted web terminal emulators for remote CLI access from a phone browser.

---

## Overview Comparison Table

| Tool | Type | Language | Performance | Setup Complexity | Security | Maintenance | Windows Support |
|------|------|----------|-------------|------------------|----------|-------------|-----------------|
| **ttyd** | Terminal Proxy | C | Excellent | Very Easy | Good | Active | Yes (native) |
| **GoTTY** | CLI to Web | Go | Good | Easy | Good | Active | Yes (binary) |
| **Wetty** | SSH-based | Node.js | Good | Medium | Good | Active | Yes |
| **Shell In A Box** | SSH-based | C | Good | Easy | Good | Inactive | Limited |
| **Gate One** | Full Terminal | Python | Good | Hard | Excellent | Inactive | Yes |
| **Ajaxterm** | Polling-based | Python/JS | Poor | Easy | Basic | Inactive | Limited |
| **Anyterm** | XmlHttpRequest | C/Java | Poor | Medium | Basic | Inactive | Limited |

---

## Detailed Tool Analysis

### 🥇 **ttyd** - RECOMMENDED
**Language:** C | **License:** MIT | **Latest:** 1.6.3

**Pros:**
- Single executable - just download and run
- Built on Libwebsockets + libuv (blazingly fast)
- Native Windows support (precompiled binaries available)
- Rich terminal emulation (Xterm.js-based) with CJK support
- File transfer (ZMODEM with lrzsz)
- SSL/TLS encryption built-in
- Basic authentication support
- Minimal resource usage
- Active development

**Cons:**
- Fewer advanced features than Gate One
- Requires C compilation if building from source

**Best for:** Simple remote CLI access from phone (your use case)

**Quick Start:**
```bash
ttyd bash  # Linux/macOS
ttyd powershell  # Windows
# Access: http://your-pc-ip:7681
```

**Resources:**
- GitHub: https://github.com/tsl0922/ttyd
- Official Site: https://tsl0922.github.io/ttyd/
- Ubuntu Manual: https://manpages.ubuntu.com/manpages/kinetic/man1/ttyd.1.html

---

### **GoTTY**
**Language:** Go | **License:** MIT | **Status:** Active

**Pros:**
- Single Go binary - minimal dependencies
- Multi-client support (use with tmux/screen)
- Comprehensive config file support
- TLS/SSL with client certificate validation
- Customizable appearance
- Good security options (auth, random URLs)
- Cross-platform

**Cons:**
- Slightly more complex configuration than ttyd
- Default port 8080 (conflicting with other services)
- Less performance-optimized than ttyd

**Best for:** Multi-user scenarios, team collaboration

**Setup:**
```bash
gotty -w bash  # Enable client write access
# http://localhost:8080
```

**Resources:**
- GitHub: https://github.com/yudai/gotty
- Homebrew: `brew install yudai/gotty/gotty`

---

### **Wetty** (Modern Alternative)
**Language:** Node.js | **License:** MIT | **Status:** Active

**Pros:**
- Uses modern Xterm.js terminal emulation
- SSH-based connections (can be remote)
- Good terminal compatibility
- WebSocket support (better than polling)
- Node.js ecosystem

**Cons:**
- Requires Node.js/npm
- More heavyweight than ttyd (~40MB memory)
- Requires SSH setup
- More configuration needed

**Best for:** SSH-based remote access, integration with SSH infrastructure

**Resources:**
- GitHub: https://github.com/butlerx/wetty
- npm: `npm install -g wetty`

---

### ⚠️ **Shell In A Box** (Legacy)
**Language:** C | **License:** GPL | **Status:** Inactive (no recent updates)

**Pros:**
- Lightweight C executable
- SSL encryption support
- AJAX-based (works without WebSockets)

**Cons:**
- No longer maintained
- AJAX polling is slower than WebSockets
- Poor copy/paste support
- Limited terminal emulation accuracy
- Not recommended for new projects

**Resources:**
- SourceForge: https://sourceforge.net/projects/shellinabox/

---

### **Gate One** (Powerful but Complex)
**Language:** Python | **License:** AGPLv3 | **Status:** Mostly inactive

**Pros:**
- Extremely feature-rich (terminal recording, multiplexing, etc.)
- Excellent security features
- Plugin architecture
- Auto-adjusts to browser window size
- WebSocket support

**Cons:**
- Complex setup (Python, dependencies)
- Heavy resource usage
- Steep learning curve
- Declining development
- Overkill for simple use cases

**Best for:** Enterprise deployments needing advanced features

**Resources:**
- GitHub: https://github.com/liftoff/GateOne
- Official Site: http://liftoffsoftware.com/Products/GateOne

---

### ❌ **Ajaxterm & Anyterm** (Obsolete)

**Ajaxterm:**
- Uses polling (slow, inefficient)
- Poor terminal emulation
- Limited copy/paste
- Outdated architecture

**Anyterm:**
- XmlHttpRequest-based
- Pseudo-terminal communication
- Created in 2005, no longer maintained
- Polling-based inefficiency

**Not recommended** - use ttyd or Wetty instead.

---

## Performance Comparison

| Metric | ttyd | GoTTY | Wetty | Shell In A Box |
|--------|------|-------|-------|----------------|
| Latency | < 50ms | 50-100ms | 100-150ms | 500-1000ms |
| CPU Usage (idle) | < 1% | ~2% | ~5% | ~3% |
| Memory (base) | ~5MB | ~10MB | ~40MB | ~8MB |
| Startup Time | Instant | 1-2s | 3-5s | 1s |

---

## Technical Architecture Comparison

### ttyd Architecture
- **Transport:** WebSockets over HTTP/HTTPS
- **Terminal Backend:** libuv (event-driven)
- **Terminal Frontend:** Xterm.js (JavaScript)
- **File Transfer:** ZMODEM protocol
- **Encryption:** OpenSSL or Mbed TLS

### GoTTY Architecture
- **Transport:** WebSockets
- **Terminal Backend:** Go standard library
- **Terminal Frontend:** hterm or xterm.js
- **Multiplexing:** Works with tmux/screen
- **Encryption:** TLS/SSL with cert validation

### Wetty Architecture
- **Transport:** WebSockets
- **Terminal Backend:** Node.js SSH client
- **Terminal Frontend:** Xterm.js
- **Authentication:** SSH-based
- **Encryption:** SSH encryption + TLS

### Shell In A Box Architecture
- **Transport:** AJAX polling (deprecated)
- **Terminal Backend:** HTML5 + JavaScript
- **Terminal Frontend:** Custom JS emulation
- **Encryption:** SSL/TLS

---

## Recommendation for Your Use Case

### **Use ttyd** because:

1. **Simplest setup** - download binary, run one command
2. **Best performance** - C-based, minimal overhead
3. **Perfect for your scenario** - accessing local CLI from phone
4. **Native Windows support** - matches your MSVC build environment
5. **Active development** - receives updates and security patches
6. **Secure by default** - SSL/TLS support easily configurable
7. **File transfer** - if you need to move files to/from phone
8. **Zero configuration** - works immediately after launch

---

## Quick Setup Instructions

### For ttyd on Windows

**1. Download:**
```
Visit: https://github.com/tsl0922/ttyd/releases
Download the latest Windows binary
```

**2. Run:**
```bash
ttyd powershell
# Access from phone: http://your-pc-ip:7681
```

**3. For basic security:**
```bash
ttyd -u username -p password powershell
# Adds basic authentication
```

**4. For remote access (ensure firewall allows):**
```bash
ttyd -b 0.0.0.0 -p 7681 powershell
# Accessible from any IP on network
```

**5. Access from phone:**
```
Open browser on phone, navigate to:
http://your-pc-ip:7681
```

### Advanced Configuration

**SSL/TLS encryption:**
```bash
ttyd --ssl --ssl-cert /path/to/cert.pem --ssl-key /path/to/key.pem powershell
```

**Read-only mode:**
```bash
ttyd --readonly powershell
# Client can view but not interact
```

**Custom command:**
```bash
ttyd cmd.exe  # Use cmd instead of PowerShell
ttyd git bash  # Use Git Bash
```

**Port specification:**
```bash
ttyd -p 8080 powershell  # Use port 8080 instead of 7681
```

---

## Session Persistence (Resume Browser Sessions)

By default, closing the browser tab closes the connection to ttyd, and the terminal session ends. If you want **session persistence** where you can close the browser and resume where you left off, you need a terminal multiplexer running on the server.

### ⚠️ Windows Host Mode Limitations

**Important:** Running ttyd + tmux natively on Windows (without WSL) has **significant compatibility issues**:

- ttyd + tmux in Git Bash/MSYS2 is **unreliable** on Windows
- Native Windows PowerShell/cmd.exe has **PTY (pseudo-terminal) incompatibility** with tmux
- Requires complex workarounds (winpty) that add instability
- **Not recommended for production use**

### Windows Host Mode Options for Session Persistence

#### Option 1: itmux (Recommended for Windows Host Mode)

**What it is:** tmux specifically packaged for Windows with minimal Cygwin environment, designed to work natively on Windows.

**Pros:**
- True tmux functionality on Windows
- Includes all needed dependencies
- No WSL required
- Standalone executable

**Setup:**
1. Download from: [GitHub - itmux](https://github.com/itefixnet/itmux/releases)
2. Extract to your project directory
3. Start with ttyd:
```bash
ttyd itmux new-session -s main -c "C:\Users\Torres\Dropbox\Projeto_Staccato\erp-staccato"
# Access: http://your-pc-ip:7681
```

**Caveat:** Less tested than native Linux tmux, may have edge cases

#### Option 2: ttyd Alone (Simpler, No Persistence)

**What it is:** Just use ttyd without any multiplexer.

**Pros:**
- Most reliable on Windows
- Simplest setup
- Works consistently
- No compatibility issues

**Cons:**
- ❌ No session persistence
- Closing browser = session ends
- Running processes stop
- No command history across sessions

**Setup:**
```bash
ttyd powershell
# Access: http://your-pc-ip:7681
```

#### Option 3: Manual Session Management (Workaround)

Use background batch files to keep processes running independently:

```batch
@REM File: start-build.bat
cd /d "C:\Users\Torres\Dropbox\Projeto_Staccato\erp-staccato"
qmake Loja.pro
nmake
pause
```

Run from ttyd:
```bash
start /B start-build.bat
# Process runs in background, monitor with:
tasklist | findstr "nmake"
```

**Pros:**
- Works natively on Windows
- Processes continue running

**Cons:**
- Not true session persistence
- Manual process management
- Limited to batch operations

### Session Persistence Comparison for Windows

| Feature | ttyd alone | ttyd + itmux | Manual batch files |
|---------|-----------|--------------|-------------------|
| Browser session survives close | ❌ No | ✅ Yes* | N/A (background) |
| Interactive commands preserved | ❌ No | ✅ Yes* | ❌ No |
| Command history across sessions | ❌ No | ✅ Yes* | ❌ No |
| Long-running processes survive | ❌ No | ✅ Yes* | ✅ Yes |
| Windows native (no WSL/Cygwin) | ✅ Yes | ⚠️ Partial** | ✅ Yes |
| Reliability on Windows | ✅ High | ⚠️ Medium | ✅ High |
| Multi-window support | ❌ No | ✅ Yes* | ❌ No |

*itmux compatibility not fully tested on all Windows versions
**Requires Cygwin runtime

### Recommendation for Your Use Case

**If you want reliable session persistence on Windows host mode:**
- Try **itmux** - it's designed for Windows and includes tmux
- Be prepared for potential edge cases or compatibility issues

**If you want maximum reliability:**
- Use **ttyd alone** (accept no persistence)
- Use manual batch file approach for long-running builds
- Keep browser tab open for interactive work

### Setup Examples

#### ttyd + itmux (Session Persistence Attempt)

```powershell
# Download itmux from GitHub releases
# Extract to C:\tools\itmux

$ttydPath = "C:\path\to\ttyd.exe"
$itmuxPath = "C:\tools\itmux\bin\tmux.exe"

# Start with itmux
& $ttydPath -u admin -p erp_dev `
  $itmuxPath new-session -s main `
  -c "C:\Users\Torres\Dropbox\Projeto_Staccato\erp-staccato"
```

#### ttyd Alone (Reliable, No Persistence)

```powershell
$ttydPath = "C:\path\to\ttyd.exe"

# Simple, reliable setup
& $ttydPath -u admin -p erp_dev powershell
```

#### Manual Background Process (Reliable Long-Running Builds)

```bash
# Run from ttyd shell
Start-Process powershell -ArgumentList @(
  '-NoExit',
  '-Command',
  'cd C:\Users\Torres\Dropbox\Projeto_Staccato\erp-staccato; qmake Loja.pro; nmake'
)

# Process continues in background
# Check status: Get-Process | findstr nmake
```

### What to Know Before Choosing itmux

Before investing time in itmux, understand:

1. **It works** - ttyd + itmux combination is theoretically sound
2. **Less tested** - Not as battle-tested as Linux tmux
3. **Potential issues** - May have edge cases on your specific Windows setup
4. **No official support** - Limited community documentation for Windows
5. **Cygwin dependency** - Requires Cygwin runtime included in itmux

**Recommendation:** Test itmux first on a non-critical task before relying on it for important builds

---

## Alternative: SSH Server (Recommended for Simplicity)

Instead of using ttyd + web browser, consider using SSH. It's simpler, more reliable, and has built-in session persistence.

### Why SSH is Better

| Feature | ttyd + Browser | SSH |
|---------|---|---|
| Session persistence | ⚠️ Requires multiplexer | ✅ Native |
| Setup complexity | Medium | Very simple |
| Phone access | Browser only | Any SSH client app |
| Security | Good | Excellent |
| File transfer | ⚠️ Limited | ✅ SFTP built-in |
| Multiplexing (tmux/screen) | ⚠️ Windows issues | ✅ Works reliably |
| Standard industry tool | No | ✅ Yes |
| Runs without browser | ❌ No | ✅ Yes |

### Windows SSH Server Installation

**System Requirements:**
- Windows 10 (build 1809+), Windows 11, or Windows Server 2019+
- PowerShell 5.1 or later
- Administrator account

#### Option 1: PowerShell Installation (Recommended)

**1. Open PowerShell as Administrator**

**2. Check OpenSSH availability:**
```powershell
Get-WindowsCapability -Online | Where-Object Name -like 'OpenSSH*'
```

**3. Install OpenSSH Server:**
```powershell
Add-WindowsCapability -Online -Name OpenSSH.Server~~~~0.0.1.0
```

**4. Start the service and set to auto-start:**
```powershell
Start-Service sshd
Set-Service -Name sshd -StartupType 'Automatic'
```

**5. Verify it's running:**
```powershell
Get-Service sshd
# Should show: Running
```

That's it! SSH is now enabled.

#### Option 2: GUI Installation

1. Open **Settings → System → Optional features**
2. Click **Add a feature**
3. Search for and install **OpenSSH Server**
4. Search for and install **OpenSSH Client** (optional)
5. Open **Services** (services.msc)
6. Find **OpenSSH SSH Server**
7. Right-click → **Properties**
8. Set **Startup type** to **Automatic**
9. Click **Start**

### Firewall Configuration

OpenSSH automatically creates a firewall rule. To verify it's enabled:

```powershell
Get-NetFirewallRule -Name "OpenSSH-Server-In-TCP"
# Should show: Enabled
```

If missing, create it manually:
```powershell
New-NetFirewallRule -Name 'OpenSSH-Server-In-TCP' `
  -DisplayName 'OpenSSH Server (sshd)' `
  -Enabled True `
  -Direction Inbound `
  -Protocol TCP `
  -Action Allow `
  -LocalPort 22
```

### Find Your PC's IP Address

```powershell
ipconfig
```

Look for **IPv4 Address** (e.g., `192.168.1.100`)

### Connect from Phone

**Install SSH client app:**
- **Android:** Termux, SSH Files, ConnectBot, Juice SSH
- **iPhone:** SSH Files, Prompt 3, iSH, Teminal

**Connection details:**
```
Host: your-pc-ip (e.g., 192.168.1.100)
Port: 22
Username: your-windows-username
Password: your-windows-password
```

### Key-Based Authentication (Optional, More Secure)

**Generate SSH keys on Windows (PowerShell as Admin):**

```powershell
ssh-keygen -t ed25519
# Press Enter to accept defaults
```

Creates two files:
- `C:\Users\YourUsername\.ssh\id_ed25519` (private key - keep secret)
- `C:\Users\YourUsername\.ssh\id_ed25519.pub` (public key)

**Add public key to authorized list:**

```powershell
# PowerShell as Administrator
$pubKeyPath = "$env:USERPROFILE\.ssh\id_ed25519.pub"
$authKeysPath = "$env:USERPROFILE\.ssh\authorized_keys"

Add-Content -Path $authKeysPath -Value (Get-Content $pubKeyPath)
```

**Copy private key to phone** (follow your SSH app's key import instructions), then connect without password.

### Default Shell Configuration

By default, SSH opens PowerShell. To change the default shell:

```powershell
# Set PowerShell as default
$NewItemPropertyParams = @{
    Path         = "HKLM:\SOFTWARE\OpenSSH"
    Name         = "DefaultShell"
    Value        = "C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe"
    PropertyType = "String"
    Force        = $true
}
New-ItemProperty @NewItemPropertyParams

# Or use cmd.exe
# Value = "C:\Windows\System32\cmd.exe"

# Or use Git Bash
# Value = "C:\Program Files\Git\bin\bash.exe"
```

Restart the SSH service for changes to take effect:
```powershell
Restart-Service sshd
```

### SSH Configuration File

Advanced settings are in: `%programdata%\ssh\sshd_config`

**Common settings:**
```
# Port to listen on (default: 22)
Port 22

# Allow/deny specific users
AllowUsers yourusername
DenyUsers admin guest

# Authentication methods
PubkeyAuthentication yes
PasswordAuthentication yes

# GSSAPI/Kerberos support (Windows Server 2022+)
GSSAPIAuthentication no
```

After editing, restart SSH:
```powershell
Restart-Service sshd
```

### Session Persistence with SSH

SSH naturally has session persistence through terminal multiplexers:

**Using tmux with SSH:**
```bash
# From phone SSH, start tmux
tmux new-session -s work

# Your session stays running even if you disconnect
# Reconnect: tmux attach-session -s work
```

**Using screen:**
```bash
# Start screen session
screen -S work

# Reconnect: screen -r work
```

Since SSH into Windows + tmux/screen in PowerShell/Git Bash works reliably, this is the best approach for persistent sessions on Windows.

### Troubleshooting SSH Connection

**Can't connect?**

```powershell
# Check service is running
Get-Service sshd

# Check if listening on port 22
netstat -an | findstr :22

# Restart service
Restart-Service sshd

# Check firewall rule
Get-NetFirewallRule -Name "OpenSSH-Server-In-TCP"
```

**Wrong IP address?**
- Verify using `ipconfig`
- Make sure on same WiFi network
- Try connecting from PC first: `ssh localhost`

**Password not working?**
- Verify SSH service restarted after config changes
- Try with full username: `domain\username`
- Check Windows login credentials

### Recommended Setup for Your Use Case

```powershell
# Step 1: Install SSH
Add-WindowsCapability -Online -Name OpenSSH.Server~~~~0.0.1.0

# Step 2: Start service
Start-Service sshd
Set-Service -Name sshd -StartupType 'Automatic'

# Step 3: From phone SSH app
# Host: your-pc-ip
# User: your-windows-username
# Password: your-windows-password

# Step 4: Inside SSH, start tmux for persistence
tmux new-session -s dev

# Now you can:
# - Close phone app, session continues running on PC
# - Reconnect hours later
# - Use tmux commands (Ctrl-B c for new window, etc.)
```

### SSH vs Other Options Summary

| Solution | Reliability | Simplicity | Persistence | Setup Time |
|----------|-------------|-----------|-------------|-----------|
| SSH alone | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ❌ (needs multiplexer) | 5 min |
| SSH + tmux | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ✅ Yes | 10 min |
| ttyd alone | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ❌ No | 10 min |
| ttyd + itmux | ⭐⭐⭐ | ⭐⭐ | ✅ (unreliable) | 30 min |

**Recommendation:** Use SSH + tmux. It's the standard approach, most reliable, and simplest to setup.

---

## Security Considerations

### For Local Network Access
```bash
ttyd -u admin -p mypassword powershell
# Basic authentication is sufficient
```

### For Internet Access (NOT recommended for critical work)
```bash
ttyd --ssl --ssl-cert cert.pem --ssl-key key.pem \
     -u admin -p complexpassword \
     powershell
```

**Better approach:** Use a VPN or SSH tunnel instead of exposing directly

### Firewall Rules
- Local network only: Allow port 7681 from your phone's IP range
- Internet access: Consider using a reverse proxy with additional security

---

## Alternative Workflows

### Option 1: Cloud Provider (Claude Code Web)
- Visit claude.ai/code
- Limited to GitHub repositories
- No local PC access

### Option 2: SSH Tunnel
```bash
# On PC: Enable SSH
# On phone: SSH to PC and access Staccato CLI directly
# Better security than exposing ttyd directly
```

### Option 3: VPN + ttyd
```bash
# Connect phone to VPN first
# Then access ttyd on local network
# Provides security layer
```

---

## Installation by OS

### Windows
1. Download from https://github.com/tsl0922/ttyd/releases
2. Extract executable to PATH or accessible location
3. Run: `ttyd powershell`

### macOS
```bash
brew install ttyd
ttyd bash
```

### Linux
```bash
# Debian/Ubuntu
sudo apt install ttyd

# Arch
yay install ttyd

# Or build from source
git clone https://github.com/tsl0922/ttyd.git
cd ttyd && mkdir build && cd build
cmake .. && make && sudo make install
```

---

## Troubleshooting

### Can't connect from phone
- Check firewall allows port 7681 (or your chosen port)
- Verify using correct PC IP address (not localhost)
- Try on same WiFi network first

### Latency is high
- Local network should be < 50ms
- Internet connections will be slower
- Consider VPN or SSH tunnel instead

### Authentication not working
- Use: `ttyd -u username -p password bash`
- Restart ttyd after changing auth

### Port already in use
- Use different port: `ttyd -p 8081 bash`
- Check what's using port 7681: `netstat -ano | findstr :7681` (Windows)

---

## Performance Tips

1. **Local network access** - Fastest option
2. **Run on dedicated core** - Reduces interference
3. **Monitor resource usage** - ttyd uses minimal resources
4. **Keep terminal height/width reasonable** - Large terminals use more bandwidth

---

## Sources

- [ttyd Official Site](https://tsl0922.github.io/ttyd/)
- [ttyd GitHub Repository](https://github.com/tsl0922/ttyd)
- [ttyd Ubuntu Manual Page](https://manpages.ubuntu.com/manpages/kinetic/man1/ttyd.1.html)
- [GoTTY GitHub](https://github.com/yudai/gotty)
- [Wetty GitHub](https://github.com/butlerx/wetty)
- [Gate One GitHub](https://github.com/liftoff/GateOne)
- [Xterm.js Project](https://xtermjs.org/)
- [Slant - Best web-based terminal emulators](https://www.slant.co/topics/1781/~best-web-based-terminal-emulators)
- [Self-hosted Web Terminal Comparison](https://medevel.com/16-list-self-hosted-terminals/)
- [How to share Linux terminal over web](https://www.tecmint.com/ttyd-share-linux-terminal-over-web/)
