# Argon2-Password-Manager
A simple password manager built in C that makes use of the powerful Argon2 hashing algorithm.

Setup
-
1. Make the necessary directories:
* cd ~
* mkdir .psswdmgr
* mkdir .psswdmgr/sites
* mkdir .psswdmgr/sites/psswd
2. Look for the variables in main.c called "psswdSalt" and "salt" and change them as desired.
3. Compile the password manager and run it.

Commands
-
* make site
* gen psswd
* exit

The "make site" command will build a JSON that will be used later to apply constraints to your password after it is generated.
* Site name refers to the name of the website. Like YouTube or Google.
* Banned characters refers to characters some websites may not allow in a password such as '%' or '=', etcetera.
* Required characters refers to the types of characters required for a password to have such as a capital letter or a number. You will choose these characters yourself.

The "gen psswd" command will generate a password for a chosen site. Please note that it is required you make a site first before using this command.
* If this is the first time gen psswd is run on a site, it will store a hashed version of the key in a file for future reference.

The "exit" command will close the program.

Note
-
* This project is a work in progress.
* This software is offered as-is and any reliance on it for password storage / maintenance / security is at your own risk. I am not liable for any losses or damages incurred through usage of this software.
