#include "action.hpp"

namespace devkit {

    static void GetAllDependsOnServices(
        AppContext& ctx,
        Service& service,
        std::vector<std::string>& outServiceNames,
        std::unordered_set<std::string>& observed) {

        if (!observed.insert(service.definition.name).second) {
            return;
        }
        for (const std::string& dependsOnServiceName : service.definition.dependsOn) {
            auto serviceOpt = ctx.GetService(dependsOnServiceName);
            if (!serviceOpt) {
                throw std::runtime_error("No such service: " + dependsOnServiceName);
            }
            GetAllDependsOnServices(ctx, **serviceOpt, outServiceNames, observed);
            if (std::find(outServiceNames.begin(), outServiceNames.end(), dependsOnServiceName) == outServiceNames.end()) {
                outServiceNames.push_back(dependsOnServiceName);
            }
        }
    }

    std::vector<std::string> GetAllDependsOnServices(AppContext& ctx, Service& service) {
        std::vector<std::string> result;
        std::unordered_set<std::string> observed;
        GetAllDependsOnServices(ctx, service, result, observed);
        return result;
    }

    static void GetAllDependsOnTasks(
        AppContext& ctx,
        Task& task,
        std::vector<std::string>& outTaskNames,
        std::unordered_set<std::string>& observed) {

        if (!observed.insert(task.GetName()).second) {
            return;
        }
        for (const std::string& dependsOnTaskName : task.GetDependsOn()) {
            auto taskOpt = ctx.GetTask(dependsOnTaskName);
            if (!taskOpt) {
                throw std::runtime_error("No such task: " + dependsOnTaskName);
            }
            GetAllDependsOnTasks(ctx, **taskOpt, outTaskNames, observed);
            if (std::find(outTaskNames.begin(), outTaskNames.end(), dependsOnTaskName) == outTaskNames.end()) {
                outTaskNames.push_back(dependsOnTaskName);
            }
        }
    }

    std::vector<std::string> GetAllDependsOnTasks(AppContext& ctx, Task& task) {
        std::vector<std::string> result;
        std::unordered_set<std::string> observed;
        GetAllDependsOnTasks(ctx, task, result, observed);
        return result;
    }
}