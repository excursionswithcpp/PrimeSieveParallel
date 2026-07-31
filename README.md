This is the code used for the post "Primes, parallelism, and caches Part 2" on my site "Excursions with C++" reachable on https://excursionswithcpp.com.
The main branch is matching the post, whereas the alternative branches contains experiments with further optimizations, of which taking over the memory allocation by the program did not give any improvements, but ignoring all the even numbers, which cannot be a prime, gave significant improvements, almost 50%.

Initializing (seeding) a new sub sieve with a partial sieve, using the result after the first few (5-7) primes, instead of all zeros, gave some improvement, 15-20%.

Outstanding experiments: 
	combining the 2 improvements
	using clang under MS VS 2026. The MS concurrency library has been ported to clang.

Try it out, copy, change and do experiments. Have fun!

