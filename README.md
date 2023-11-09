# json-thresher
C++ json binder

## To build and test

Note: currently working on a new package manager to use on this package.  Other managers should still 
work(as far as they do) as https://github.com/DataDriven-CAM/cnpm.git is based on the package.json npm format.

### cnpm building

```
cnpm  install

cnpm lib

#building and running unit tests
cnpm test

```
## Examples

```
    sylvanmats::io::json::Path jpSymbol;
    jpSymbol["elements"]["*"]["symbol"]=="C";

```
matches any object in the elements array where symbol is C; and from periodic table [Periodic-Table-JSON](https://github.com/Bowserinator/Periodic-Table-JSON.git) 
and traverses all value pairs for the element by ```jsonBinder(jpName, [&](std::string_view& key, std::any& v){});```.  

## Contact

My twitch stream is https://www.twitch.tv/sylvanmats
(with some hardware hope to get active)