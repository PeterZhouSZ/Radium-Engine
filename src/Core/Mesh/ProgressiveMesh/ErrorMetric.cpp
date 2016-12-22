#include <Core/Mesh/ProgressiveMesh/ErrorMetric.hpp>

#include <Core/Geometry/Triangle/TriangleOperation.hpp>

#include <Core/Mesh/DCEL/Iterator/Edge/EFIterator.hpp>
#include <Core/Mesh/DCEL/Iterator/Face/FFIterator.hpp>

#include <Core/Log/Log.hpp>

namespace Ra
{
    namespace Core
    {

        //---------------------------------------------------

        QuadricErrorMetric::QuadricErrorMetric()
        {
        }

        QuadricErrorMetric::QuadricErrorMetric(Scalar scale)
        {
            m_param.scale = scale;
        }

        Scalar QuadricErrorMetric::computeGeometricError(const Primitive& q, const Primitive::Vector& p)
        {
            // Computing geometric error
            // v^T A v + 2 * b^T v + c
            Eigen::Matrix<Scalar, 1, 3> row_p = p.transpose();
            Eigen::Matrix<Scalar, 1, 3> row_b = q.getB().transpose();
            Scalar error_a = row_p * q.getA() * p;
            Scalar error_b = 2.0 * row_b * p;
            Scalar error_c = q.getC();
            return (error_a + error_b + error_c);
        }

        Scalar QuadricErrorMetric::computeError(const Primitive& q, const Vector3& vs, const Vector3& vt, Vector3& pResult)
        {
            Scalar error;

            // on cherche v_result
            // A v_result = -b		avec A = nn^T
            //							 b = dn
            Primitive::Matrix AInverse = q.getA().inverse();
            Primitive::Vector result;

            Scalar det = q.getA().determinant();
            if (det > 0.0001)
            {
                result = -AInverse * q.getB();
                error = computeGeometricError(q, result);
            }
            else //matrix non inversible
            {
                Primitive::Vector p1  = vs;
                Primitive::Vector p2  = vt;
                Primitive::Vector p12 = (p1 + p2) / 2.0;

                Scalar p1_error     = computeGeometricError(q, p1);
                Scalar p2_error     = computeGeometricError(q, p2);
                Scalar p12_error    = computeGeometricError(q, p12);

                error = p1_error;
                Primitive::Vector p = p1;
                if (p2_error < error && p12_error > p2_error)
                {
                    p = p2;
                    result = p;
                    error = p2_error;
                }
                else if (p12_error < error && p2_error > p12_error)
                {
                    p = p12;
                    result = p;
                    error = p12_error;
                }
                else
                {
                    result = p;
                }
            }
            pResult = Vector3(result.x(), result.y(), result.z());
            return error;
        }

        void QuadricErrorMetric::generateFacePrimitive(Primitive &q, const Face_ptr f, Dcel &dcel)
        {
            Vertex_ptr v0 = f->HE()->V();
            Vertex_ptr v1 = f->HE()->Next()->V();
            Vertex_ptr v2 = f->HE()->Next()->Next()->V();

            Primitive::Vector n = Geometry::triangleNormal(v0->P(), v1->P(), v2->P());
            q = Primitive(n, -n.dot(v0->P()));
        }

        //---------------------------------------------------

        APSSErrorMetric::APSSErrorMetric()
        {
        }

        APSSErrorMetric::APSSErrorMetric(Scalar scale)
        {
            m_param.scale = scale;
        }

        Scalar APSSErrorMetric::computeGeometricError(const Primitive& q, const Primitive::Vector& p)
        {
            // Computing geometric error
            // v^T A v + 2 * b^T v + c
            /*
            Scalar uc = std::sqrt(q.getC());
            Vector3 ul = Vector3(q.getB().x(), q.getB().y(), q.getB().z()) / uc;
            Scalar uq = q.getB().w();
            Vector3 p2 = Vector3(p.x(), p.y(), p.z());
            return uc + p2.dot(ul) + uq * p2.squaredNorm();
            */

            Eigen::Matrix<Scalar, 1, 4> row_p = p.transpose();
            Eigen::Matrix<Scalar, 1, 4> row_b = q.getB().transpose();
            Scalar error_a = row_p * q.getA() * p;
            Scalar error_b = 2.0 * row_b * p;
            Scalar error_c = q.getC();
            return (error_a + error_b + error_c);

        }

        Scalar APSSErrorMetric::computeError(const Primitive& q, const Vector3& vs, const Vector3& vt, Vector3& pResult)
        {
            Scalar error;

            // on cherche v_result
            // A v_result = -b		avec A = nn^T
            //							 b = dn
            Primitive::Matrix AInverse = q.getA().inverse();
            Primitive::Vector vsPrimitiveType = Primitive::Vector(vs.x(), vs.y(), vs.z(), vs.norm() * vs.norm());
            Primitive::Vector vtPrimitiveType = Primitive::Vector(vt.x(), vt.y(), vt.z(), vt.norm() * vt.norm());
            Primitive::Vector result;

            Scalar det = q.getA().determinant();
            /*
            if (det > 0.0001)
            {
                result = -AInverse * q.getB();
                error = computeGeometricError(q, result);
            }
            else //matrix non inversible
            {
                Primitive::Vector p1  = vsPrimitiveType;
                Primitive::Vector p2  = vtPrimitiveType;
                Primitive::Vector p12 = (p1 + p2) / 2.0;

                Scalar p1_error     = computeGeometricError(q, p1);
                Scalar p2_error     = computeGeometricError(q, p2);
                Scalar p12_error    = computeGeometricError(q, p12);

                error = p1_error;
                Primitive::Vector p = p1;
                if (p2_error < error && p12_error > p2_error)
                {
                    p = p2;
                    result = p;
                    error = p2_error;
                }
                else if (p12_error < error && p2_error > p12_error)
                {
                    p = p12;
                    result = p;
                    error = p12_error;
                }
                else
                {
                    result = p;
                }
            }
            */
            Primitive::Vector p1  = vsPrimitiveType;
            Primitive::Vector p2  = vtPrimitiveType;
            Primitive::Vector p12 = Primitive::Vector((vs.x() + vt.x()) / 2.0,
                                                      (vs.y() + vt.y()) / 2.0,
                                                      (vs.z() + vt.z()) / 2.0,
                                                      (vs + vt).norm() * (vs + vt).norm());

            Scalar p1_error     = computeGeometricError(q, p1);
            Scalar p2_error     = computeGeometricError(q, p2);
            Scalar p12_error    = computeGeometricError(q, p12);
            result = p12;
            error = p12_error;

            pResult = Vector3(result.x(), result.y(), result.z());

            return error;
        }

        void APSSErrorMetric::generateFacePrimitive(Primitive &q, Face_ptr f, Dcel &dcel)
        {
            Vertex_ptr v0 = f->HE()->V();
            Vertex_ptr v1 = f->HE()->Next()->V();
            Vertex_ptr v2 = f->HE()->Next()->Next()->V();
            Vector3 p = (v0->P() + v1->P() + v2->P()) / 3.0;
            GrenaillePoint::VectorType pg = GrenaillePoint::VectorType(p.x(), p.y(), p.z());

            Fit1 fit;
            fit.setWeightFunc(WeightFunc());

            fit.init(pg);

            GrenaillePoint::VectorType pgi;
            FFIterator ffIt = FFIterator(f);
            FaceList adjFaces = ffIt.list();

            for (uint i = 0; i < adjFaces.size(); i++)
            {
                Face_ptr fi = adjFaces[i];
                v0 = fi->HE()->V();
                v1 = fi->HE()->Next()->V();
                v2 = fi->HE()->Next()->Next()->V();
                p = (v0->P() + v1->P() + v2->P()) / 3.0;
                pgi = GrenaillePoint::VectorType(p.x(), p.y(), p.z());
                Vector3 n = Geometry::triangleNormal(v0->P(), v1->P(), v2->P());
                GrenaillePoint gpi(pgi, n);
                fit.addNeighbor(gpi);
            }

            fit.finalize();

            if (fit.isStable())
            {
                //fit.applyPrattNorm();
                //q = fit;
                m_param.fits.push_back(fit);
            }
            else
            {
                CORE_ASSERT(fit.isStable(), "APSS FIT IS NOT STABLE");
            }
        }

        //---------------------------------------------------

        SimpleAPSSErrorMetric::SimpleAPSSErrorMetric()
        {
        }

        SimpleAPSSErrorMetric::SimpleAPSSErrorMetric(Scalar scale)
        {
            m_param.scale = scale;
        }

        Scalar SimpleAPSSErrorMetric::computeGeometricError(const Primitive& q, const Vector3& p)
        {

        }

        /*
        SimpleAPSSErrorMetric::Primitive SimpleAPSSErrorMetric::combine(const Primitive& a, const Primitive& b)
        {
            Primitive c = a;
            c.changeBasis(b.basisCenter());
            c.applyPrattNorm();

            //c.setParameters((c.tau() + b.tau())/Scalar(2), (c.eta() + b.eta()), (c.kappa() + b.kappa())/Scalar(2));
            c.setParameters((c.tau() + b.tau()), (c.eta() + b.eta()), (c.kappa() + b.kappa()));

            c.changeBasis(a.basisCenter());
            c.applyPrattNorm();
            return c;
        }
        */

        SimpleAPSSErrorMetric::Primitive SimpleAPSSErrorMetric::combine(const Primitive& a, const Scalar& a_weight, const Primitive& b, const Scalar& b_weight, std::ofstream &file)
        {
            Primitive c;
            Scalar new_uc   =   0.5*a_weight*b_weight*(a.m_ul.dot(b.m_ul)) +
                                a_weight*a_weight*a.m_uc*b.m_uq +
                                b_weight*b_weight*b.m_uc*a.m_uq -
                                0.5*a_weight*b_weight*a.prattNorm()*b.prattNorm();
            Vector3 new_ul  = a_weight*a.m_ul*b.m_uq + b_weight*b.m_ul*a.m_uq;
            Scalar new_uq   = a.m_uq*b.m_uq;
            Vector3 new_p   = a_weight*a.m_p + b_weight*b.m_p;
            Scalar test = new_ul.squaredNorm() - Scalar(4.) * new_uc * new_uq;

            /*
            if ((a.m_uc > 0 && b.m_uc < 0) || (a.m_uc < 0 && b.m_uc > 0))
            {
                if ((a.m_uq > 0 && b.m_uq < 0) || (a.m_uq < 0 && b.m_uq > 0))
                {
                    if (((a.m_ul.x() > 0 && b.m_ul.x() < 0) || (a.m_ul.x() < 0 && b.m_ul.x() > 0)) &&
                        ((a.m_ul.y() > 0 && b.m_ul.y() < 0) || (a.m_ul.y() < 0 && b.m_ul.y() > 0)) &&
                        ((a.m_ul.z() > 0 && b.m_ul.z() < 0) || (a.m_ul.z() < 0 && b.m_ul.z() > 0)))
                    {
                        LOG(logINFO) << "inversion de signes";
                    }
                }
            }
            */

            if (test > -0.00000000001 && test < 0.00000000001)
            {
                LOG(logINFO) << "ca merde ici : norm 0 \n";
                file << a.m_uc  << " " << a.m_ul.transpose()  << " " << a.m_uq << " " << a.m_p.transpose() << "\n";
                file << a.center().x() << " " << a.center().y() << " " << a.center().z() << " " << a.radius() << "\n";
                file << b.m_uc  << " " << b.m_ul.transpose()  << " " << b.m_uq << " " << b.m_p.transpose() << "\n";
                file << b.center().x() << " " << b.center().y() << " " << b.center().z() << " " << b.radius() << "\n";
            }
            if (std::abs(test) > 10000000000000)
            {
                LOG(logINFO) << "ca merde ici : norm inf \n";
                file << a.m_uc  << " " << a.m_ul.transpose()  << " " << a.m_uq << " " << a.m_p.transpose() << "\n";
                file << a.center().x() << " " << a.center().y() << " " << a.center().z() << " " << a.radius() << "\n";
                file << b.m_uc  << " " << b.m_ul.transpose()  << " " << b.m_uq << " " << b.m_p.transpose() << "\n";
                file << b.center().x() << " " << b.center().y() << " " << b.center().z() << " " << b.radius() << "\n";
            }
            if (test < 0)
            {
                LOG(logINFO) << "ca merde ici : norm negative \n";
                file << a.m_uc  << " " << a.m_ul.transpose()  << " " << a.m_uq << " " << a.m_p.transpose() << "\n";
                file << a.center().x() << " " << a.center().y() << " " << a.center().z() << " " << a.radius() << "\n";
                file << b.m_uc  << " " << b.m_ul.transpose()  << " " << b.m_uq << " " << b.m_p.transpose() << "\n";
                file << b.center().x() << " " << b.center().y() << " " << b.center().z() << " " << b.radius() << "\n";
            }
            c.setParameters(new_uc, new_ul, new_uq, new_p);
            return c;
        }

        SimpleAPSSErrorMetric::Primitive SimpleAPSSErrorMetric::combine(const Primitive& a, const Scalar& a_weight, const Primitive& b, const Scalar& b_weight)
        {
            Primitive c;
            if (a.m_uq > -0.00000000001 && a.m_uq < 0.00000000001 &&
                   b.m_uq > -0.00000000001 && b.m_uq < 0.00000000001 )
                LOG(logINFO) << "plan";
            Scalar new_uc   =   0.5*a_weight*b_weight*(a.m_ul.dot(b.m_ul)) +
                                a_weight*a_weight*a.m_uc*b.m_uq +
                                b_weight*b_weight*b.m_uc*a.m_uq -
                                0.5*a_weight*b_weight*a.prattNorm()*b.prattNorm();
            Vector3 new_ul  = a_weight*a.m_ul*b.m_uq + b_weight*b.m_ul*a.m_uq;
            Scalar new_uq   = a.m_uq*b.m_uq;
            Vector3 new_p   = a_weight*a.m_p + b_weight*b.m_p;
            Scalar test = new_ul.squaredNorm() - Scalar(4.) * new_uc * new_uq;
            if (test > -0.00000000001 && test < 0.00000000001)
            {
                LOG(logINFO) << "ca merde ici : norm 0 \n";
            }
            if (std::abs(test) > 10000000000000)
            {
                LOG(logINFO) << "ca merde ici : norm inf \n";
            }
            if (test < 0)
            {
                LOG(logINFO) << "ca merde ici : norm negative \n";
            }
            c.setParameters(new_uc, new_ul, new_uq, new_p);
            return c;
        }

        Scalar SimpleAPSSErrorMetric::computeError(Primitive& q, const Vector3& vs, const Vector3& vt, Vector3& pResult)
        {
            GrenaillePoint::VectorType center = q.center();
            Scalar radius = q.radius();
            GrenaillePoint::VectorType p12 = (vs + vt) / 2.0;
            pResult = q.project(p12);
            Scalar error = std::abs(q.potential(p12));
            return error;

            // Projection of p12 on the sphere
            /*
            Vector3 pSphere = Vector3(p12.x() - center.x(),
                                      p12.y() - center.y(),
                                      p12.z() - center.z());

            Scalar pSphereLength = pSphere.norm();
            Vector3 qSphere = pSphere * (radius / pSphereLength);
            Vector3 rSphere = Vector3(qSphere.x() + center.x(),
                                      qSphere.y() + center.y(),
                                      qSphere.z() + center.z());
            pResult = rSphere;
            return (p12 - rSphere).norm();
            */
        }

        void SimpleAPSSErrorMetric::generateFacePrimitive(Primitive &q, Face_ptr f, Dcel &dcel, Scalar mean_edge_size, Scalar scale)
        {
            Vertex_ptr v0 = f->HE()->V();
            Vertex_ptr v1 = f->HE()->Next()->V();
            Vertex_ptr v2 = f->HE()->Next()->Next()->V();
            Vector3 p = (v0->P() + v1->P() + v2->P()) / 3.0;
            GrenaillePoint::VectorType pg = GrenaillePoint::VectorType(p.x(), p.y(), p.z());
            GrenaillePoint::VectorType new_pg = GrenaillePoint::VectorType(p.x(), p.y(), p.z());

            Fit1 fit;
            fit.setWeightFunc(WeightFunc(mean_edge_size)); // TODO weight func

            Scalar error;
            FFIterator ffIt = FFIterator(f);
            std::set<Face_ptr, FFIterator::compareFacePtr> adjFacesSet;
            ffIt.nRing(std::ceil((scale/mean_edge_size) + 1), adjFacesSet); // TODO N-ring
            //ffIt.nRing(1, adjFacesSet); // TODO N-ring
            do {
                fit.init(new_pg);
                GrenaillePoint::VectorType pgi;
                std::set<Face_ptr, FFIterator::compareFacePtr>::iterator it;
                for (it = adjFacesSet.begin(); it != adjFacesSet.end(); ++it)
                {
                    Face_ptr fi = *it;
                    v0 = fi->HE()->V();
                    v1 = fi->HE()->Next()->V();
                    v2 = fi->HE()->Next()->Next()->V();
                    p = (v0->P() + v1->P() + v2->P()) / 3.0;
                    pgi = GrenaillePoint::VectorType(p.x(), p.y(), p.z());
                    Vector3 n = Geometry::triangleNormal(v0->P(), v1->P(), v2->P());
                    GrenaillePoint::VectorType ng = GrenaillePoint::VectorType(n.x(), n.y(), n.z());
                    GrenaillePoint gpi(pgi, ng);
                    fit.addNeighbor(gpi);
                }

                fit.finalize();
                fit.applyPrattNorm();
                new_pg = fit.project(pg);
                error = (new_pg-pg).norm();
                pg = new_pg;
            } while (error > 0.01); // TODO threshold

            if (fit.getCurrentState() != UNDEFINED)
            {
                q = fit;
                m_param.fits.push_back(fit);
            }
            else
            {
                CORE_ASSERT(true, "APSS FIT IS NOT STABLE");
            }
        }

    }
}


