git init
git remote add origin https://github.com/AV99999/verilog_parser_gui.git
git add .
git commit -m "First Upload"
git config --global user.name "AV99999"
git config --global user.email "av990990@gmail.com"
git commit -m "First Upload"
git branch -M main
git push -u origin main
git branch -M main
git push -u origin main
l
git pull origin main --allow-unrelated-histories
git add .
git commit -m "Merge remote main into local"
git push -u origin main
git pull origin main --allow-unrelated-histories
git pull origin main --allow-unrelated-histories --no-rebase
git add .
git commit -m "Merged remote main into local"
git push -u origin main
git log

#save password
git config --global credential.helper store


#delete hist bad sensitive info
sudo apt install git-filter-repo
git filter-repo --path git_hash.info --invert-paths --force

