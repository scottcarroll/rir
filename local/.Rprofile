tests <- function() {
}



packageTests <- function() {
   cat("I AM IN UR TESTZ\n")
   testsHome = "../rjit/tests"
   for (file in list.files(testsHome)) {
       cat("    ")
       cat(file)
       cat("\n")
       source(paste(testsHome, file, sep="/"))
   }
   cat("IM IN UR TESTZ\n")
}

.First <- function() {
   cat("OH HAI! CAN I HAZ LIBRARIES\n")
   library("compiler")
   dyn.load("../build/librir.so")
   cat("IM IN UR DLLZ\n")
   source("../rir/R/rir.R")
   cat("IM IN UR PACKAGE\n")


   cat("AWSHUM\n")
}

.Last <- function() {
   cat("KTHXBYE!\n")
}
