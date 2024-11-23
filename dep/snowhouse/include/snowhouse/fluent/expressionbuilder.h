//          Copyright Joakim Karlsson & Kim Gräsman 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SNOWHOUSE_EXPRESSIONBUILDER_H
#define SNOWHOUSE_EXPRESSIONBUILDER_H

#include "../constraints/constraints.h"
#include "constraintadapter.h"
#include "operators/andoperator.h"
#include "operators/notoperator.h"
#include "operators/oroperator.h"
#include "operators/collections/alloperator.h"
#include "operators/collections/noneoperator.h"
#include "operators/collections/atleastoperator.h"
#include "operators/collections/exactlyoperator.h"
#include "operators/collections/atmostoperator.h"

namespace snowhouse
{
    // ---- Evaluation of list of constraints

    template <typename ConstraintListType, typename ActualType>
    inline void EvaluateConstraintList(ConstraintListType& constraint_list,
        ResultStack& result,
        OperatorStack& operators,
        const ActualType& actual)
    {
        constraint_list.m_head.Evaluate(
            constraint_list, result, operators, actual);
    }

    template <typename ActualType>
    inline void EvaluateConstraintList(
        Nil&, ResultStack&, OperatorStack&, const ActualType&)
    {
    }

    template <typename ConstraintListType>
    struct ExpressionBuilder
    {
        explicit ExpressionBuilder(const ConstraintListType& list):
            m_constraint_list(list)
        {
        }

        template <typename ExpectedType>
        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<ConstraintAdapter<EqualsConstraint<ExpectedType>>,
                Nil>>::t>
        EqualTo(const ExpectedType& expected)
        {
            using ConstraintAdapterType =
                ConstraintAdapter<EqualsConstraint<ExpectedType>>;
            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    ConstraintList<ConstraintAdapterType, Nil>>::t>;

            ConstraintAdapterType constraint(expected);
            ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());

            return BuilderType(Concatenate(m_constraint_list, node));
        }

        template <typename ExpectedType, typename DeltaType>
        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<
                ConstraintAdapter<
                    EqualsWithDeltaConstraint<ExpectedType, DeltaType>>,
                Nil>>::t>
        EqualToWithDelta(const ExpectedType& expected, const DeltaType& delta)
        {
            using ConstraintAdapterType = ConstraintAdapter<
                EqualsWithDeltaConstraint<ExpectedType, DeltaType>>;
            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    ConstraintList<ConstraintAdapterType, Nil>>::t>;

            ConstraintAdapterType constraint(
                EqualsWithDeltaConstraint<ExpectedType, DeltaType>(
                    expected, delta));
            ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());

            return BuilderType(Concatenate(m_constraint_list, node));
        }

        template <typename MatcherType>
        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<ConstraintAdapter<FulfillsConstraint<MatcherType>>,
                Nil>>::t>
        Fulfilling(const MatcherType& matcher)
        {
            using ConstraintAdapterType =
                ConstraintAdapter<FulfillsConstraint<MatcherType>>;
            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    ConstraintList<ConstraintAdapterType, Nil>>::t>;

            ConstraintAdapterType constraint(matcher);
            ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());

            return BuilderType(Concatenate(m_constraint_list, node));
        }

        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<ConstraintAdapter<EqualsConstraint<bool>>, Nil>>::t>
        False()
        {
            return EqualTo<bool>(false);
        }

        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<ConstraintAdapter<EqualsConstraint<bool>>, Nil>>::t>
        True()
        {
            return EqualTo<bool>(true);
        }

        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<ConstraintAdapter<EqualsConstraint<std::nullptr_t>>,
                Nil>>::t>
        Null()
        {
            return EqualTo<std::nullptr_t>(nullptr);
        }

        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<ConstraintAdapter<EqualsConstraint<std::string>>,
                Nil>>::t>
        EqualTo(const char* expected)
        {
            return EqualTo<std::string>(std::string(expected));
        }

        template <typename ExpectedType>
        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<
                ConstraintAdapter<IsGreaterThanConstraint<ExpectedType>>,
                Nil>>::t>
        GreaterThan(const ExpectedType& expected)
        {
            using ConstraintAdapterType =
                ConstraintAdapter<IsGreaterThanConstraint<ExpectedType>>;

            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    ConstraintList<ConstraintAdapterType, Nil>>::t>;
            ConstraintAdapterType constraint(expected);
            ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        template <typename ExpectedType>
        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<ConstraintAdapter<
                               IsGreaterThanOrEqualToConstraint<ExpectedType>>,
                Nil>>::t>
        GreaterThanOrEqualTo(const ExpectedType& expected)
        {
            using ConstraintAdapterType = ConstraintAdapter<
                IsGreaterThanOrEqualToConstraint<ExpectedType>>;

            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    ConstraintList<ConstraintAdapterType, Nil>>::t>;
            ConstraintAdapterType constraint(expected);
            ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        template <typename ExpectedType>
        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<
                ConstraintAdapter<IsLessThanConstraint<ExpectedType>>,
                Nil>>::t>
        LessThan(const ExpectedType& expected)
        {
            using ConstraintAdapterType =
                ConstraintAdapter<IsLessThanConstraint<ExpectedType>>;

            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    ConstraintList<ConstraintAdapterType, Nil>>::t>;
            ConstraintAdapterType constraint(expected);
            ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        template <typename ExpectedType>
        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<
                ConstraintAdapter<IsLessThanOrEqualToConstraint<ExpectedType>>,
                Nil>>::t>
        LessThanOrEqualTo(const ExpectedType& expected)
        {
            using ConstraintAdapterType =
                ConstraintAdapter<IsLessThanOrEqualToConstraint<ExpectedType>>;

            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    ConstraintList<ConstraintAdapterType, Nil>>::t>;
            ConstraintAdapterType constraint(expected);
            ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        template <typename ExpectedType>
        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<ConstraintAdapter<ContainsConstraint<ExpectedType>>,
                Nil>>::t>
        Containing(const ExpectedType& expected)
        {
            using ConstraintAdapterType =
                ConstraintAdapter<ContainsConstraint<ExpectedType>>;

            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    ConstraintList<ConstraintAdapterType, Nil>>::t>;
            ConstraintAdapterType constraint(expected);
            ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<ConstraintAdapter<ContainsConstraint<std::string>>,
                Nil>>::t>
        Containing(const char* expected)
        {
            return Containing<std::string>(std::string(expected));
        }

        template <typename ExpectedType>
        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<ConstraintAdapter<EndsWithConstraint<ExpectedType>>,
                Nil>>::t>
        EndingWith(const ExpectedType& expected)
        {
            using ConstraintAdapterType =
                ConstraintAdapter<EndsWithConstraint<ExpectedType>>;
            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    ConstraintList<ConstraintAdapterType, Nil>>::t>;

            ConstraintAdapterType constraint(expected);
            ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<ConstraintAdapter<EndsWithConstraint<std::string>>,
                Nil>>::t>
        EndingWith(const char* expected)
        {
            return EndingWith(std::string(expected));
        }

        template <typename ExpectedType>
        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<
                ConstraintAdapter<StartsWithConstraint<ExpectedType>>,
                Nil>>::t>
        StartingWith(const ExpectedType& expected)
        {
            using ConstraintAdapterType =
                ConstraintAdapter<StartsWithConstraint<ExpectedType>>;

            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    ConstraintList<ConstraintAdapterType, Nil>>::t>;
            ConstraintAdapterType constraint(expected);
            ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<ConstraintAdapter<StartsWithConstraint<std::string>>,
                Nil>>::t>
        StartingWith(const char* expected)
        {
            return StartingWith(std::string(expected));
        }

        template <typename ExpectedType>
        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<ConstraintAdapter<HasLengthConstraint<ExpectedType>>,
                Nil>>::t>
        OfLength(const ExpectedType& expected)
        {
            using ConstraintAdapterType =
                ConstraintAdapter<HasLengthConstraint<ExpectedType>>;

            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    ConstraintList<ConstraintAdapterType, Nil>>::t>;
            ConstraintAdapterType constraint(expected);
            ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<ConstraintAdapter<IsEmptyConstraint>, Nil>>::t>
        Empty()
        {
            using ConstraintAdapterType = ConstraintAdapter<IsEmptyConstraint>;

            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    ConstraintList<ConstraintAdapterType, Nil>>::t>;
            ConstraintAdapterType constraint(0);
            ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        template <typename ExpectedType>
        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<
                ConstraintAdapter<EqualsContainerConstraint<ExpectedType,
                    bool (*)(const typename ExpectedType::value_type&,
                        const typename ExpectedType::value_type&)>>,
                Nil>>::t>
        EqualToContainer(const ExpectedType& expected)
        {
            using DefaultBinaryPredicateType =
                bool (*)(const typename ExpectedType::value_type&,
                    const typename ExpectedType::value_type&);
            using ConstraintAdapterType =
                ConstraintAdapter<EqualsContainerConstraint<ExpectedType,
                    DefaultBinaryPredicateType>>;

            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    ConstraintList<ConstraintAdapterType, Nil>>::t>;
            ConstraintAdapterType constraint(
                EqualsContainerConstraint<ExpectedType,
                    DefaultBinaryPredicateType>(expected,
                    constraint_internal::default_comparer<
                        typename ExpectedType::value_type>));
            ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        template <typename ExpectedType, typename BinaryPredicate>
        ExpressionBuilder<typename type_concat<ConstraintListType,
            ConstraintList<
                ConstraintAdapter<
                    EqualsContainerConstraint<ExpectedType, BinaryPredicate>>,
                Nil>>::t>
        EqualToContainer(
            const ExpectedType& expected, const BinaryPredicate predicate)
        {
            using ConstraintAdapterType = ConstraintAdapter<
                EqualsContainerConstraint<ExpectedType, BinaryPredicate>>;

            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    ConstraintList<ConstraintAdapterType, Nil>>::t>;
            ConstraintAdapterType constraint(
                EqualsContainerConstraint<ExpectedType, BinaryPredicate>(
                    expected, predicate));
            ConstraintList<ConstraintAdapterType, Nil> node(constraint, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        using AndOperatorNode = ConstraintList<AndOperator, Nil>;
        using OrOperatorNode = ConstraintList<OrOperator, Nil>;
        using NotOperatorNode = ConstraintList<NotOperator, Nil>;
        using AllOperatorNode = ConstraintList<AllOperator, Nil>;
        using AtLeastOperatorNode = ConstraintList<AtLeastOperator, Nil>;
        using ExactlyOperatorNode = ConstraintList<ExactlyOperator, Nil>;
        using AtMostOperatorNode = ConstraintList<AtMostOperator, Nil>;
        using NoneOperatorNode = ConstraintList<NoneOperator, Nil>;

        ExpressionBuilder<
            typename type_concat<ConstraintListType, AllOperatorNode>::t>
        All()
        {
            using BuilderType = ExpressionBuilder<
                typename type_concat<ConstraintListType, AllOperatorNode>::t>;
            AllOperator op;
            AllOperatorNode node(op, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        ExpressionBuilder<
            typename type_concat<ConstraintListType, AtLeastOperatorNode>::t>
        AtLeast(unsigned int expected)
        {
            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    AtLeastOperatorNode>::t>;
            AtLeastOperator op(expected);
            AtLeastOperatorNode node(op, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        ExpressionBuilder<
            typename type_concat<ConstraintListType, ExactlyOperatorNode>::t>
        Exactly(unsigned int expected)
        {
            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    ExactlyOperatorNode>::t>;
            ExactlyOperator op(expected);
            ExactlyOperatorNode node(op, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        ExpressionBuilder<
            typename type_concat<ConstraintListType, AtMostOperatorNode>::t>
        AtMost(unsigned int expected)
        {
            using BuilderType =
                ExpressionBuilder<typename type_concat<ConstraintListType,
                    AtMostOperatorNode>::t>;
            AtMostOperator op(expected);
            AtMostOperatorNode node(op, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        ExpressionBuilder<
            typename type_concat<ConstraintListType, NoneOperatorNode>::t>
        None()
        {
            using BuilderType = ExpressionBuilder<
                typename type_concat<ConstraintListType, NoneOperatorNode>::t>;
            NoneOperator op;
            NoneOperatorNode node(op, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        ExpressionBuilder<
            typename type_concat<ConstraintListType, AndOperatorNode>::t>
        And()
        {
            using BuilderType = ExpressionBuilder<
                typename type_concat<ConstraintListType, AndOperatorNode>::t>;
            AndOperator op;
            AndOperatorNode node(op, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        ExpressionBuilder<
            typename type_concat<ConstraintListType, OrOperatorNode>::t>
        Or()
        {
            using BuilderType = ExpressionBuilder<
                typename type_concat<ConstraintListType, OrOperatorNode>::t>;
            OrOperator op;
            OrOperatorNode node(op, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        ExpressionBuilder<
            typename type_concat<ConstraintListType, NotOperatorNode>::t>
        Not()
        {
            using BuilderType = ExpressionBuilder<
                typename type_concat<ConstraintListType, NotOperatorNode>::t>;
            NotOperator op;
            NotOperatorNode node(op, Nil());
            return BuilderType(Concatenate(m_constraint_list, node));
        }

        template <typename ActualType>
        void Evaluate(ResultStack& result,
            OperatorStack& operators,
            const ActualType& actual)
        {
            EvaluateConstraintList(
                m_constraint_list, result, operators, actual);
        }

        ConstraintListType m_constraint_list;
    };

    template <typename T>
    inline void StringizeConstraintList(const T& list, std::ostringstream& stm)
    {
        if (stm.tellp() > 0)
            stm << " ";

        stm << snowhouse::Stringize(list.m_head);
        StringizeConstraintList(list.m_tail, stm);
    }

    inline void StringizeConstraintList(const Nil&, std::ostringstream&) {}

    template <typename ConstraintListType>
    struct Stringizer<ExpressionBuilder<ConstraintListType>>
    {
        static std::string ToString(
            const ExpressionBuilder<ConstraintListType>& builder)
        {
            std::ostringstream stm;
            StringizeConstraintList(builder.m_constraint_list, stm);

            return stm.str();
        }
    };
}

#endif
