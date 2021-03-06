% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/mcmcObject.R
\name{runMCMC}
\alias{runMCMC}
\title{Run MCMC}
\usage{
runMCMC(mcmc, genome, model, ncores = 1, divergence.iteration = 0)
}
\arguments{
\item{mcmc}{MCMC object that will run the model fitting algorithm.}

\item{genome}{Genome that the model fitting will run on. Should be 
the same genome associated with the parameter and model objects.}

\item{model}{Model to run the fitting on. Should be associated with
the given genome.}

\item{ncores}{Number of cores to perform the model fitting with. Default
value is 1.}

\item{divergence.iteration}{Number of steps that the initial conditions
can diverge from the original conditions given. Default value is 0.}
}
\value{
This function has no return value.
}
\description{
\code{runMCMC} will run a monte carlo markov chain algorithm
for the given mcmc, genome, and model objects to perform a model fitting.
}
\details{
\code{runMCMC} will run for the number of samples times the number
thining given when the mcmc object is initialized. Updates are provided every 100
steps, and the state of the chain is saved every thining steps.
}

