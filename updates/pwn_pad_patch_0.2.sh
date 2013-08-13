#!/bin/bash

echo "Removing Ruby 1.8 and installing Metasploit bundle"
apt-get remove ruby1.8 ruby1.8-dev libruby1.8 rubygems
gem install bundler --no-ri --no-rdoc
cd /opt/metasploit-framework && bundle install
echo "Updating Metasploit"
msfupdate
