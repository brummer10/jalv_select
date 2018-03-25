# LOCALISATION

## jalv.select supports gettext for localisation

If you wish to add support for your language, simply run the following command from command-line:  
`make po`  
It will ask you some questions, answer them.  
That will generate your language .po file in the ./po directory. Now open it with your preferred translator app (suggestion: Gtranslator) and translate the strings.  
### suggestion: chose from the menu to insert original strings first, and translate them then, that will help to keep the formatting intact.

When done, save your work and run:  
`make`  
to generate the .mo file.  
In order to use your new translation, you only need to run:  
`make install`

You could as well generate .po files for other then your locale language, by running:  
`make po LANG=lang`  
were lang is one of the outputs from  
`locale -a`  

### If you wish to contribute your localisation upstream, please open a issue on the github tracker, or open a pull request.
