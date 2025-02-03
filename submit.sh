#!/bin/bash

# "Before the final submission:
#   -perform a make clean
#   -keep latest source checked out in your directory
#   -create your programs in a directory called username.1
#   -username is your login name on opsys
#   -remove the executables and object files
#   -set perms to owner only
#   -copy repo as "username.1" to "/home/hauschildm/cs4760/assignment1/"

HOMEUSER="hauschildm"
CLASS="cs4760"
USERNAME=$(logname)

if [ $# -ne 1 ]; then
	echo "Usage: $0 <assignmentNumber>"
	exit 1
fi

PROJECT_NUM="$1"

if [ ! -d "p${PROJECT_NUM}" ]; then
	echo "Error: Directory p${PROJECT_NUM} does not exist."
	exit 1
fi

git pull

echo "Updating Makefile: setting PROJECT ?= $PROJECT_NUM"
sed -i "s/^PROJECT ?= .*/PROJECT ?= $PROJECT_NUM/" Makefile

if [ -f "p${PROJECT_NUM}/README" ]; then
	echo "Overwriting root README with p${PROJECT_NUM}/README..."
	cp "p${PROJECT_NUM}/README" "README"
fi

echo "PROJECT=$PROJECT_NUM make clean..."
PROJECT=$PROJECT_NUM make clean || true

echo "Finding remote version for /home/$HOMEUSER/$CLASS/assignment$PROJECT_NUM..."
N=1
while [ -d "/home/$HOMEUSER/$CLASS/assignment$PROJECT_NUM/$USERNAME.$N" ]; do
	((N++))
done
DIRNAME="$USERNAME.$N"

echo "Creating local directory $DIRNAME..."
rm -rf "$DIRNAME"
mkdir "$DIRNAME"

echo "Copying repo..."
for f in .??* *; do
	if [ "$f" = "$DIRNAME" ] || [ "$f" = "." ] || [ "$f" = ".." ]; then
		continue
	fi
	cp -rp "$f" "$DIRNAME/"
done

echo "Setting directory permissions..."
chmod -R 700 "$DIRNAME"

echo "Copying $DIRNAME to /home/$HOMEUSER/$CLASS/assignment$PROJECT_NUM..."
mkdir -p "/home/$HOMEUSER/$CLASS/assignment$PROJECT_NUM"
cp -rp "$DIRNAME" "/home/$HOMEUSER/$CLASS/assignment$PROJECT_NUM/"

echo "Removing local $DIRNAME..."
rm -rf "$DIRNAME"

echo "Submission complete: /home/$HOMEUSER/$CLASS/assignment$PROJECT_NUM/$DIRNAME"
