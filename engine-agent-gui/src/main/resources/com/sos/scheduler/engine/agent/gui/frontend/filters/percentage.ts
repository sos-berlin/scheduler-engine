/// <reference path='../_all.ts' />
(function (angular) {
    angular.module('agent')
        .filter('percentage', ['$filter', function ($filter) {
            return (input, decimals) => $filter('number')(input * 100, decimals) + '%'
        }])
}(angular))
