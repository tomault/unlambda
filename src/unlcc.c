/** Implementation of the Unlambda compiler.
 * 
 *  To compile an expression of the form `xy, do the following
 *  1. Output code for a one-argument function f(z) that evaluates x
 *     to u, calls z() to obtain v, and then calls u(v) and returns the
 *     result.
 *  2. Output code for a no-argument function g() that evaluates y
 *     and returns the result.
 *  3. Output code to call f(g)
 *
 *  To compile s, k, i, v, c, d or r, simply output code to push the address
 *     of the combinator implementation onto the address stack.
 *
 *  To compile .<ch>, compile a function fx(z) of one argument that does the
 *     following:
 *         1.  Call z() to evaluate the argument
 *         2.  Print <ch>
 *         3.  Return the value computed in step (1)
 *
 *  The implementation of the different combinators is as follows:
 *  * k[x] = (lambda z.lambda y.((lambda u.z)(y())))(x())
 *  * s[x] = (lambda u.lambda y.((
 *               lambda v.lambda z.(lambda w.(u(w)(v(w))))(z())
 *           )(y())))(x())
 *  * i[x] = x()
 *  * v[x] = (lambda y.v)(x())
 *  * d[x] = lambda y.(x()(y()))
 *  * c[x] = {
 *        let cc = continuation to c1
 *        return x()(cc)
 *   } c1(y) {
 *        return y()
 *   }
 *  * .[c, x] = (lambda y.(print[c](y)))(x())
 *  * r[x] = (lambda y.(print['\n'](y)))(x())
 *
 * Where print[c](x) is a special function that prints unicode character c
 * then returns x
 */

