# Guide for Translation
Please note that if you contribute a translation, you be asked to maintain it, unless you say otherwise.
1. Download the template translation file [here](https://github.com/sevenc-nanashi/MikuMikuWorld4CC/tree/main/MikuMikuWorld/res/i18n).
2. Fill in each line with the appropriate translation. 
For `update_available_description`, `division` and `division_affix`, there are special codes which are replaced. For `update_available_description`, `%s` is replaced with the version. For `division` and `division_affix`, `%d` is replaced with the division. If you are confused, please look at the other translations.
3. Rename the file to be the 2 letter language code followed by .csv. Language codes are listed [here](https://en.wikipedia.org/wiki/List_of_ISO_639_language_codes).
4. Add the file to the folder `MikuMikuWorld/res/i18n` and restart MikuMikuWorld. The new language should appear in the options. Confirm that every piece of text is right, and change if needed, then restart MMW.
5. If you know how, fork the repo and create a pull request. If not, then send the translation file to @moplo on Discord, and they will add it for you.