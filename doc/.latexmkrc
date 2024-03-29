#$pdflatex = "xelatex -src-specials -synctex=1 -interaction=nonstopmode %O %S";

$pdf_previewer = "start evince %O %S";

# for nomenclature
add_cus_dep("nlo", "nls", 0, "nlo2nls");
sub nlo2nls {
    system("makeindex $_[0].nlo -s nomencl.ist -o $_[0].nls -t $_[0].nlg");
}
