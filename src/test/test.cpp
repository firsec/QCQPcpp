#include "qcqpcpp/mosekOptProblem.h"
#include "qcqpcpp/bonminOptProblem.h"

inline qcqpcpp::MosekOpt<double>::BOUND toSGBound( MSKboundkeye bound )
{
    return qcqpcpp::MosekOpt<double>::BOUND( bound );
}

int testQC()
{
    typedef double      Scalar;
    typedef MSKrescodee ReturnType;

    const int numcon = 1; /* Number of constraints. */
    const int numvar = 3; /* Number of variables. */
    //const int numanz = 3; /* Number of non-zeros in A. */
    const int numqnz = 4; /* Number of non-zeros in Q. */

    MSKrescodee r;

    double q[] = {0.0,-1.0,0.0};

    MSKboundkeye bkc[] = {MSK_BK_LO};
    double blc[] = {1.0};
    double buc[] = {+MSK_INFINITY};

    MSKboundkeye bkx[] = {MSK_BK_LO,
                          MSK_BK_LO,
                          MSK_BK_LO};
    double blx[] = {0.0,
                    0.0,
                    0.0};
    double bux[] = {+MSK_INFINITY,
                    +MSK_INFINITY,
                    +MSK_INFINITY};

    MSKint32t aptrb[] = { 0 },
              aptre[] = { 3 },
              asub [] = { 0, 1, 2 };

    // x >= 0
    double aval[] = { 1.0, 1.0, 1.0 };
    MSKint32t qsubi[numqnz],
              qsubj[numqnz];
    double    qval[numqnz];

    qsubi[0] = 0; qsubj[0] = 0; qval[0] = 2.0;
    qsubi[1] = 1; qsubj[1] = 1; qval[1] = 0.2;
    qsubi[2] = 2; qsubj[2] = 0; qval[2] = -1.0;
    qsubi[3] = 2; qsubj[3] = 2; qval[3] = 2.0;

    qcqpcpp::MosekOpt<Scalar>::SparseMatrix QObj( numvar, numvar );
    QObj.insert(0,0) = 2.0;
    QObj.insert(1,1) = 0.2;
    QObj.insert(2,0) = -1.0;
    QObj.insert(2,2) = 2.0;

    qcqpcpp::MosekOpt<Scalar>::SparseMatrix QCon( numvar, numvar );
    QCon.insert(0,0) = -2.0;
    QCon.insert(1,1) = -2.0;
    QCon.insert(2,2) = -0.2;
    QCon.insert(2,0) =  0.2;

#if 1
    // test mosek wrapper
    std::vector<qcqpcpp::MosekOpt<Scalar>::Scalar> x_out, gt_out;
    {
        qcqpcpp::MosekOpt<Scalar> mosek;

        // set vars
        for ( int j = 0; j != numvar; ++j )
        {
            // add var
            mosek.addVariable( toSGBound(bkx[j]), blx[j], bux[j] );
            // obj function set
            mosek.setLinObjective( j, q[j] );
        }

        // set quadratic objective
        mosek.addQObjectives( QObj );

        // set constraints
        for ( int i = 0; i != numcon; ++i )
        {
            std::cout << "numcon: " << numcon << ", aptre[i] - aptrb[i]: " << aptre[i] - aptrb[i] << std::endl;
            // add var
            std::vector<double> coeffs( numvar );
            for ( int cid = 0; cid != aptre[i] - aptrb[i]; ++cid )
            {
                std::cout << "setting coeffs[" << asub[aptrb[i] + cid] << "] = aval[" <<  aptrb[i] + cid << "];" << std::endl;
                std::cout << "\t~==   coeffs[ asub[" << aptrb[i] << " + " << cid << "] ] = aval[ aptrb[" << i<< "] + " << cid << "];" << std::endl;
                coeffs[ asub[aptrb[i] + cid] ] = aval[ aptrb[i] + cid ];
            }
            std::cout<<"coeffs:";for(size_t vi=0;vi!=coeffs.size();++vi)std::cout<<coeffs[vi]<<" ";std::cout << "\n";
            mosek.addLinConstraint( toSGBound(bkc[i]), blc[i], buc[i], coeffs );
        }

        // set QConstraints
        {
            for ( int k = 0; k < QCon.outerSize(); ++k )
            {
                for ( qcqpcpp::MosekOpt<Scalar>::SparseMatrix::InnerIterator it(QCon, k); it; ++it )
                {
                    mosek.addQConstraint( 0, it.row(), it.col(), it.value() );
                    std::cout << "adding constraint " << it.row() << "," << it.col() << ", " << it.value() << std::endl;
                }
            }
        }

        int r = mosek.update();
        if ( r == MSK_RES_OK )
            std::cout << "mosek.finalize returns: " << r << ", MSK_RES_OK: " << (int)MSK_RES_OK << std::endl;
        else
            std::cerr << "mosek.finalize returns: " << r << ", MSK_RES_OK: " << (int)MSK_RES_OK << std::endl;

        mosek.printProblem();
        mosek.optimize( &x_out );
        std::cout<<"x_out:";for(size_t vi=0;vi!=x_out.size();++vi)std::cout<<x_out[vi]<<" ";std::cout << "\n";
        std::cout << std::endl << std::endl;
    }
#endif

    MSKint32t j,i;
    double xx[numvar];
    MSKenv_t env;
    MSKtask_t task;

    /* Create the mosek environment. */
    r = MSK_makeenv(&env,NULL);

    if ( r==MSK_RES_OK )
    {
        /* Create the optimization task. */
        r = MSK_maketask(env,numcon,numvar,&task);

        if ( r==MSK_RES_OK )
        {
            r = MSK_linkfunctotaskstream(task,MSK_STREAM_LOG,NULL,qcqpcpp::mosekPrintStr);

            /* Append 'NUMCON' empty constraints.
   The constraints will initially have no bounds. */
            if ( r == MSK_RES_OK )
            {
                std::cout << "gt: MSK_appendcons(task,"<<numcon<<");" << std::endl;
                r = MSK_appendcons(task,numcon);
            }

            /* Append 'NUMVAR' variables.
   The variables will initially be fixed at zero (x=0). */
            if ( r == MSK_RES_OK )
            {
                std::cout << "gt: MSK_appendvars(task,"<<numvar<<");" << std::endl;
                r = MSK_appendvars(task,numvar);
            }

            /* Optionally add a constant term to the objective. */
            if ( r ==MSK_RES_OK )
            {
                std::cout << "gt: MSK_putcfix(task," << 0.0 << ");" << std::endl;
                r = MSK_putcfix(task,0.0);
            }
            for(j=0; j<numvar && r == MSK_RES_OK; ++j)
            {
                /* Set the linear term c_j in the objective.*/
                if(r == MSK_RES_OK)
                {
                    std::cout << "gt: putcj(task," << j << "," << q[j] << ")" << std::endl;
                    r = MSK_putcj(task,j,q[j]);
                }

                /* Set the bounds on variable j.
   blx[j] <= x_j <= bux[j] */
                if(r == MSK_RES_OK)
                {
                    r = MSK_putvarbound(task,
                                        j, /* Index of variable.*/
                                        bkx[j], /* Bound key.*/
                                        blx[j], /* Numerical value of lower bound.*/
                                        bux[j]); /* Numerical value of upper bound.*/
                    std::cout << "gt: MSK_putvarbound(task," << j << "," << bkx[j] << "," << blx[j] << "," << bux[j] << ");" << std::endl;
                }

                /* Input column j of A */
//                if(r == MSK_RES_OK)
//                {
//                    r = MSK_putacol(task,
//                                    j, /* Variable (column) index.*/
//                                    aptre[j]-aptrb[j], /* Number of non-zeros in column j.*/
//                                    asub+aptrb[j], /* Pointer to row indexes of column j.*/
//                                    aval+aptrb[j]); /* Pointer to Values of column j.*/
//                }
            }

            /* Set the bounds on constraints.
   for i=1, ...,NUMCON : blc[i] <= constraint i <= buc[i] */
            for(i=0; i<numcon && r==MSK_RES_OK; ++i)
            {
//                r = MSK_putarow(task,
//                                j, /* Variable (column) index.*/
//                                aptre[j]-aptrb[j], /* Number of non-zeros in column j.*/
//                                asub+aptrb[j], /* Pointer to row indexes of column j.*/
//                                aval+aptrb[j]); /* Pointer to Values of column j.*/

                std::cout << "gt: MSK_putarow( task, " << i
                          << ", " << aptre[i] - aptrb[i]
                             << ", " << *(asub + aptrb[i])
                             << ", " << *(aval + aptrb[i])
                             << ");" << std::endl;
                r = MSK_putarow(task,
                                i,                 /* Row index.*/
                                aptre[i]-aptrb[i], /* Number of non-zeros in row i.*/
                                asub+aptrb[i],     /* Pointer to column indexes of row i.*/
                                aval+aptrb[i]);    /* Pointer to values of row i.*/

                std::cout << "my: MSK_putconbound( task, " << i
                          << ", " << bkc[i]
                          << ", " << blc[i]
                          << ", " << buc[i]
                          << ")" << std::endl;
                r = MSK_putconbound(task,
                                    i, /* Index of constraint.*/
                                    bkc[i], /* Bound key.*/
                                    blc[i], /* Numerical value of lower bound.*/
                                    buc[i]); /* Numerical value of upper bound.*/
            }

            if ( r==MSK_RES_OK )
            {
                /*
   * The lower triangular part of the Q^o
   * matrix in the objective is specified.
   */

//                qsubi[0] = 0; qsubj[0] = 0; qval[0] = 2.0;
//                qsubi[1] = 1; qsubj[1] = 1; qval[1] = 0.2;
//                qsubi[2] = 2; qsubj[2] = 0; qval[2] = -1.0;
//                qsubi[3] = 2; qsubj[3] = 2; qval[3] = 2.0;

                /* Input the Q^o for the objective. */
                std::cout<<"gt: MSK_putqobj(task," << numqnz << ",\n";
                for(size_t vi=0;vi!=numqnz;++vi)
                {
                    std::cout << qsubi[vi] << "," << qsubj[vi] << ", " << qval[vi] << std::endl;
                }
                std::cout << ");\n";
                r = MSK_putqobj(task,numqnz,qsubi,qsubj,qval);

            }

            if ( r==MSK_RES_OK )
            {
                /*
   * The lower triangular part of the Q^0
   * matrix in the first constraint is specified.
   This corresponds to adding the term
   - x_1^2 - x_2^2 - 0.1 x_3^2 + 0.2 x_1 x_3
   */

                qsubi[0] = 0; qsubj[0] = 0; qval[0] = -2.0;
                qsubi[1] = 1; qsubj[1] = 1; qval[1] = -2.0;
                qsubi[2] = 2; qsubj[2] = 2; qval[2] = -0.2;
                qsubi[3] = 2; qsubj[3] = 0; qval[3] = 0.2;

                /* Put Q^0 in constraint with index 0. */

                std::cout<<"gt: MSK_putqonk( task, " << 0
                        << ", " << 4
                        << ",\n";
                for(size_t vi=0;vi!=4;++vi)
                {
                    std::cout << qsubi[vi] << "," << qsubj[vi] << ", " << qval[vi] << std::endl;
                }
                std::cout << "); " << std::endl;
                r = MSK_putqconk(task,
                                 0,
                                 4,
                                 qsubi,
                                 qsubj,
                                 qval);
            }

            if ( r==MSK_RES_OK )
                r = MSK_putobjsense(task, MSK_OBJECTIVE_SENSE_MINIMIZE);

            if ( r==MSK_RES_OK )
            {
                MSKrescodee trmcode;

                /* Run optimizer */
                r = MSK_optimizetrm(task,&trmcode);

                /* Print a summary containing information
   about the solution for debugging purposes*/
                MSK_solutionsummary (task,MSK_STREAM_LOG);

                if ( r==MSK_RES_OK )
                {
                    MSKsolstae solsta;
                    int j;

                    MSK_getsolsta(task,MSK_SOL_ITR,&solsta);

                    switch(solsta)
                    {
                        case MSK_SOL_STA_OPTIMAL:
                        case MSK_SOL_STA_NEAR_OPTIMAL:
                            MSK_getxx(task,
                                      MSK_SOL_ITR, /* Request the interior solution. */
                                      xx);

                            printf("Optimal primal solution\n");
                            gt_out.clear(); gt_out.reserve(numvar);
                            for(j=0; j<numvar; ++j)
                            {
                                gt_out.push_back( xx[j] );
                                printf("x[%d]: %e\n",j,xx[j]);
                            }

                            break;
                        case MSK_SOL_STA_DUAL_INFEAS_CER:
                        case MSK_SOL_STA_PRIM_INFEAS_CER:
                        case MSK_SOL_STA_NEAR_DUAL_INFEAS_CER:
                        case MSK_SOL_STA_NEAR_PRIM_INFEAS_CER:
                            printf("Primal or dual infeasibility certificate found.\n");
                            break;

                        case MSK_SOL_STA_UNKNOWN:
                            printf("The status of the solution could not be determined.\n");
                            break;
                        default:
                            printf("Other solution status.");
                            break;
                    }
                }
                else
                {
                    printf("Error while optimizing.\n");
                }
            }

            if (r != MSK_RES_OK)
            {
                /* In case of an error print error code and description. */
                char symname[MSK_MAX_STR_LEN];
                char desc[MSK_MAX_STR_LEN];

                printf("An error occurred while optimizing.\n");
                MSK_getcodedesc (r,
                                 symname,
                                 desc);
                printf("Error %s - '%s'\n",symname,desc);
            }
        }

        MSK_deletetask(&task);
    }
    MSK_deleteenv(&env);

    int passed = 0;
    std::cout<<"x_out:\n";
    for ( size_t vi = 0; vi != x_out.size(); ++vi )
    {
        if ( x_out[vi] != gt_out[vi] )
            ++passed;
        std::cout << "\tx(" << vi << ")=\t" << x_out[vi] << ((x_out[vi] == gt_out[vi]) ? " =OK= " : " =!NOT!= ") << gt_out[vi] << "(gt)" << std::endl;
    }
    std::cout << "\n";

    return ( r + passed );
}

int testBonmin()
{
    typedef double Scalar;

    qcqpcpp::BonminOpt<Scalar> problem;
    problem.printSolutionAtEndOfAlgorithm();

    Eigen::Matrix<Scalar,-1,1> qo(4,1); qo << 100.2714996337891, 100.6660919189453
                                            , 100.4376068115234, 101.2251129150391;
    for ( int j = 0; j != qo.rows(); ++j )
        problem.addVariable( qcqpcpp::OptProblem<Scalar>::BOUND::RANGE, Scalar(0), Scalar(1), qcqpcpp::OptProblem<Scalar>::VAR_TYPE::BINARY );

    problem.setLinObjectives( qo );
    std::cout<<"[" << __func__ << "]: " << "linobj ok" << std::endl; fflush(stdout);
    qcqpcpp::BonminOpt<Scalar>::SparseMatrix Qo(4,4);
    Qo.insert( 0, 1 ) = 0.5713889002799988;
    Qo.insert( 1, 2 ) = 0.5713889002799988;
    Qo.insert( 0, 3 ) = 0.9172399044036865;
    Qo.insert( 1, 3 ) = 0.7175261378288269;
    Qo.insert( 2, 3 ) = 0.9172399044036865;
    problem.setQObjectives( Qo );
    std::cout<<"[" << __func__ << "]: " << "qobj ok" << std::endl; fflush(stdout);

    qcqpcpp::BonminOpt<Scalar>::SparseMatrix A(2,4);
    A.insert( 0, 0 ) = 1;
    A.insert( 0, 3 ) = 1;
    A.insert( 1, 1 ) = 1;
    A.insert( 1, 2 ) = 1;
    problem.addLinConstraint( qcqpcpp::OptProblem<Scalar>::BOUND::GREATER_EQ, Scalar(1), problem.getINF(), /* coeffs: */ NULL );
    problem.addLinConstraint( qcqpcpp::OptProblem<Scalar>::BOUND::GREATER_EQ, Scalar(1), problem.getINF(), /* coeffs: */ NULL );
    problem.setLinConstraints( A );
    std::cout<<"[" << __func__ << "]: " << "lincon ok" << std::endl; fflush(stdout);

    problem.update( true );
    std::vector<Scalar> x_out;
    problem.optimize( &x_out, qcqpcpp::OptProblem<Scalar>::MINIMIZE );
    // expected outcome: 1 0 1 0;
    const std::vector<Scalar> x_gt = { 1, 0, 1, 0};
    if ( std::equal(x_out.begin(), x_out.end(), x_gt.begin()) )
    {
        std::cout << "[" << __func__ << "]: " << "BonminTest PASSED" << std::endl;
        return 0;
    }
    else
    {
        std::cerr << "[" << __func__ << "]: " << "BonminTest FAILED" << std::endl;
        return 1;
    }
}

int main( int argc, char **argv )
{
    typedef double Scalar;
    int err = EXIT_SUCCESS;

    err += testBonmin();
    err += testQC();

    if ( err == EXIT_SUCCESS )
        std::cout << "[" << __func__ << "]: " << "all tests PASSED" << std::endl;
    else
        std::cerr << "[" << __func__ << "]: " << err << " tests FAILED" << std::endl;

    return err;
}

