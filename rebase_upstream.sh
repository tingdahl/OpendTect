#!/bin/bash

UPSTREAM=upstream

if (( $# != 1 )); then
    >&2 echo "Syntax: $0 <branch>"
    exit 1
fi

BRANCH=$1

#Check that git exists
if ! command -v git &> /dev/null
then
    echo "git could not be found"
    exit 1
fi


#Check that grep exists
if ! command -v grep &> /dev/null
then
    echo "grep could not be found"
    exit 1
fi


#Check if upstream exists
if ! git remote | grep ${UPSTREAM} &> /dev/null; then
    echo "Adding ${UPSTREAM} repo"
    git remote add ${UPSTREAM} https://github.com/OpendTect/OpendTect.git
fi

if ! git remote | grep ${UPSTREAM} &> /dev/null; then
    echo "Upstram repo is not available and cannot be added"
    exit 1
fi


echo "Fetching ${UPSTREAM}"
if ! git fetch ${UPSTREAM} &> /dev/null; then
    echo "Cannot fetch ${UPSTREAM}"
    exit 1
fi


echo "Checking out ${BRANCH}"
if ! git checkout ${BRANCH} &> /dev/null; then
    echo "Cannot checkout ${BRANCH}"
    exit 1
fi

echo "Checking if latest upstream commit is in local history"
if git log ${BRANCH} --pretty=format:"%H" | grep `git log -n 1 ${UPSTREAM}/${BRANCH} --pretty=format:"%H"` >> /dev/null; then
    echo Local branch is up to date, nothing to do
    exit 0
fi


echo "Rebasing on ${UPSTREAM}/${BRANCH}"
if ! git rebase ${UPSTREAM}/${BRANCH} &> /dev/null; then
    echo "Cannot rebase ${UPSTREAM}/${BRANCH}"
    exit 1
fi

echo "Pushing rebase to origin"
if ! git push origin --force &> /dev/null; then
    echo "Cannot push origin"
    exit 1
fi

echo "Cleaning build"
if ! snapcraft clean; then
    echo "Could not clean build"
    exit 1
fi

echo "Removing previous artifacts"
rm *.snap


echo "Building"
if ! snapcraft pack; then
    echo "Could not pack build"
    exit 1
fi

snapname=`ls *.snap`
echo "Uploading new snap"
export SNAPCRAFT_STORE_CREDENTIALS=$(cat ~/Documents/snapcraft.login)
if ! snapcraft upload ${snapname} --release=edge; then
    echo "Could not upload snap"
    exit 1
fi

md5=($(md5sum ${snapname}))
archivesnapname=${md5}-${snapname}
echo "Archiving snap as ${archivesnapname}"
mv ${snapname} snap_archive/$archivesnapname
