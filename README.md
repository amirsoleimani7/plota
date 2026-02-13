# Plota — Online Othello (Reversi) + Real-Time Chat

Plota is a full-stack application featuring an **online Othello (Reversi) game** with **real-time room creation** and **mid-game chat**. It includes a complete authentication flow (**Sign up / Sign in / Forgot password**) and a **profile editor**, packaged with a clean UI and partial responsiveness.

Built with **Qt 6.10.2** and **MinGW 64-bit** toolchain.

---

## ✨ Features

### ✅ Authentication & Accounts
- Sign up / Sign in
- Forgot password flow
- Profile edit (user info updates)

### 🎮 Online Othello (Reversi)
- Create and join online game rooms
- Real-time gameplay experience
- Match flow suitable for online play

### 💬 In-Game Chat
- Chat while playing (mid-game)
- Room-based messaging

### 🎨 UI
- Modern UI
- Responsive to some extent (works well on common sizes)

---

## 🖼️ Screenshots

> Screenshots are located in: `ScreenShots/`

### Login Page
![Login](ScreenShots/login-page.png) 

### Main Menu
![Main Menu](ScreenShots/main-menu.png) 

### Othello Game
![Online Othello Chat](ScreenShots/online-othello-chat.png) 

---

## 🧱 Project Structure

This repository contains **both client and server**:

```
plota/
├── PlotaClient/          # Qt-based game client
├── PlotaServer/          # Qt-based game server
├── installers/           # Pre-built installers
│   ├── PlotaClientSetup.exe
│   └── PlotaServerSetup.exe
└── ScreenShots/          # Application screenshots
```

---

## 🚀 Getting Started

### Option 1: Use Pre-built Installers

The easiest way to run Plota is using the pre-built installers:

1. **Download installers** from the `installers/` directory:
   - `PlotaClientSetup.exe` (~37.7 MB) - Game client installer
   - `PlotaServerSetup.exe` (~8.7 MB) - Game server installer

2. **Install the server** first by running `PlotaServerSetup.exe`

3. **Install the client** by running `PlotaClientSetup.exe`

4. **Run the server**, then launch the client to start playing!

### Option 2: Use Release Builds

Pre-compiled release builds are available in:
- **Server**: `PlotaServer/PlotaServer/build/Desktop_Qt_6_10_2_MinGW_64_bit-Release/`
- **Client**: `PlotaClient/PlotaClient/build/Desktop_Qt_6_10_2_MinGW_64_bit-Release/`

Simply navigate to these directories and run the executables directly.

### Option 3: Build from Source

#### Prerequisites
- **Qt 6.10.2** or later
- **MinGW 64-bit** toolchain
- **CMake** (for build configuration)

#### Build Steps

1. **Clone the repository**:
   ```bash
   git clone https://github.com/amirsoleimani7/plota.git
   cd plota
   ```

2. **Open in Qt Creator**:
   - Open `PlotaServer/PlotaServer/PlotaServer.pro` (or CMakeLists.txt)
   - Open `PlotaClient/PlotaClient/PlotaClient.pro` (or CMakeLists.txt)

3. **Configure the kit**:
   - Select **Desktop Qt 6.10.2 MinGW 64-bit** as the build kit

4. **Build**:
   - Build the server first
   - Then build the client

5. **Run**:
   - Start the server executable
   - Launch the client executable

---

## 🛠️ Technology Stack

- **Framework**: Qt 6.10.2
- **Language**: C++ (96.8%), CMake (3.2%)
- **Compiler**: MinGW 64-bit
- **Networking**: Qt Network module for real-time communication
- **UI**: Qt Widgets / Qt Quick (QML)

---

## 📦 Releases

Pre-built installers are available in the `installers/` directory for quick setup without needing to build from source.

---

## 📄 License

This project is open source. Please check the repository for license details.

---

## 🤝 Contributing

Contributions, issues, and feature requests are welcome! Feel free to check the issues page.

---

Enjoy playing Plota! 🎮✨