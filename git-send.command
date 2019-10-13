#!/bin/sh

cd $HOME/Documents/Arduino
whoami=`whoami`
message="Commit by $whoami"
echo $message | echo "git commit -a $whoami --file=- --author=$whoami"


