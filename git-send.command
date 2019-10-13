#!/bin/sh

cd $HOME/Documents/Arduino
whoami=`whoami`
message="Commit by $whoami"
echo $message | git commit -a --author=$whoami --file=- --author=$whoami
git push origin master


