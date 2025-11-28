# 1. Change directory to your project folder
cd C:/Users/SURYA/Documents/wifi-car-remote-control

# 2. Initialize a new Git repository in the current folder
git init

# 3. Add all files (including subdirectories like server/ and esp8266/)
git add .

# 4. Commit the added files with an initial message
git commit -m "Initial commit: Adding dual-mode Wi-Fi car remote control project files"

# 5. Connect your local repository to your GitHub repository
git remote add origin https://github.com/vsurya2011/wifibot.git

# 6. Set the branch name to 'main' (standard practice)
git branch -M main

# 7. Push the local 'main' branch content to the remote 'origin' (GitHub)
# You will be prompted to enter your GitHub username and Personal Access Token (PAT) here.
git push -u origin main