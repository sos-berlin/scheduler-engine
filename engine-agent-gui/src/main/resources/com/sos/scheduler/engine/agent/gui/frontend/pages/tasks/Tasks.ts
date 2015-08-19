/// <reference path="../../_all.ts" />
angular.module("agent")
.controller("Tasks", ["$scope", "$rootScope", "AgentRestangular", ($scope, $rootScope, AgentRest: restangular.IService) => {
    $scope.update = (): void => {
        AgentRest.one('task').get().then(response => {
            $scope.taskHandler = response.data
            $scope.taskHandler.tasks.sort((a, b) => a.id.localeCompare(b.id))
            $scope.now = moment.utc(response.headers('Date'), "ddd, DD MMM YYYY HH:mm:ss [GMT]")
        })
    }
    $scope.$on("Update", () => $scope.update())
    $rootScope.$broadcast("Update")
}])
