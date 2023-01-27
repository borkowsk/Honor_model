# Honor Culture Model
## Source code for model for paper: "The Evolutionary Basis of Honor Cultures"

Paper location: https://journals.sagepub.com/doi/10.1177/0956797615602860

Data etc.: https://osf.io/btawq/ , https://osf.io/x8g6z/

On RG: https://www.researchgate.net/publication/284727943_The_Evolutionary_Basis_of_Honor_Cultures

If you are using this code for research purposes, please quote this article.
If to any of the others, give the coffee to the author 
(https://www.paypal.com/paypalme/wborkowsk)

## Compilation

You need a __SymshellLight library__ for building this project.
It is expected in `../../public/SymShellLight` in relation to `CMakeLists.txt`.
You need to edit such file apropriatelly, if source code of the __SymShellLight__ 
is in diferent location.

## Usage

`./honor -help` - graphics options

`./honor HELP` - model options

`./honor` - run the model with default settings

`./honor <list of options>` - run the model with particular settings


## POSSIBLE COMMANDS FOR GRAPHIC WINDOW:

* q - q-uit or ESC

* n - n-ext MC step
* p - p-ause simulation
* r - r-un simulation

* b - save screen to B-MP
* d - d-ump on every visualisation

* 1..0 visualisation frequency
* c - swich c-onsole on/off
* s - visualise s-hort links
* f - visualise f-ar links
* a - visualise a-gents
* e - visualise d-e-cisions
* u - visualise rep-u-tations

* mouse left or right - 
     means inspection
     
* ENTER - replot screen


## License

CC-BY-NC-4.0

https://spdx.org/licenses/CC-BY-NC-4.0.html


