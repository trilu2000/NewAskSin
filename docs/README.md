# Doxygen documentation

In order to work with the API documentation, the html subdir must 
be inititalized once:
 mkdir html
 cd html
 git clone git@github.com:trilu2000/NewAskSin.git -b gh-pages

To update the doxygen documentation, perform these steps in the 
local clone of the repository:
 (cd html; git pull)
 doxygen docs/Doxyfile
 (cd html && git add . && git commit -m 'Update API docs' && git push) 
