/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "Thread System", "index.html", [
    [ "System Overview", "index.html#overview", null ],
    [ "Key Features", "index.html#features", null ],
    [ "Architecture Diagram", "index.html#architecture", null ],
    [ "Quick Start", "index.html#quickstart", null ],
    [ "Installation", "index.html#installation", [
      [ "CMake FetchContent (Recommended)", "index.html#install_fetchcontent", null ],
      [ "vcpkg", "index.html#install_vcpkg", null ],
      [ "Build from Source", "index.html#install_manual", null ]
    ] ],
    [ "Module Overview", "index.html#modules", null ],
    [ "Learning Resources", "index.html#learning", null ],
    [ "Examples", "index.html#examples", null ],
    [ "Related Systems", "index.html#related", null ],
    [ "Tutorial: Thread Pool", "tutorial_threadpool.html", [
      [ "Introduction", "tutorial_threadpool.html#tutorial_tp_intro", null ],
      [ "When to Use thread_pool", "tutorial_threadpool.html#tutorial_tp_when_basic", null ],
      [ "When to Use typed_thread_pool", "tutorial_threadpool.html#tutorial_tp_when_typed", null ],
      [ "Work-Stealing Configuration", "tutorial_threadpool.html#tutorial_tp_workstealing", null ],
      [ "Sizing the Pool", "tutorial_threadpool.html#tutorial_tp_sizing", null ],
      [ "Example 1: Basic Thread Pool", "tutorial_threadpool.html#tutorial_tp_example1", null ],
      [ "Example 2: Typed Thread Pool with Priorities", "tutorial_threadpool.html#tutorial_tp_example2", null ],
      [ "Example 3: Sizing for an I/O-Bound Workload", "tutorial_threadpool.html#tutorial_tp_example3", null ],
      [ "Next Steps", "tutorial_threadpool.html#tutorial_tp_next", null ]
    ] ],
    [ "Tutorial: DAG Scheduling", "tutorial_dag.html", [
      [ "Introduction", "tutorial_dag.html#tutorial_dag_intro", null ],
      [ "Defining Dependencies", "tutorial_dag.html#tutorial_dag_dependencies", null ],
      [ "Execution Order Guarantees", "tutorial_dag.html#tutorial_dag_order", null ],
      [ "Error Handling", "tutorial_dag.html#tutorial_dag_errors", null ],
      [ "Example 1: Linear Pipeline", "tutorial_dag.html#tutorial_dag_example1", null ],
      [ "Example 2: Fan-out / Fan-in", "tutorial_dag.html#tutorial_dag_example2", null ],
      [ "Example 3: Best-Effort Failure Handling", "tutorial_dag.html#tutorial_dag_example3", null ],
      [ "Next Steps", "tutorial_dag.html#tutorial_dag_next", null ]
    ] ],
    [ "Tutorial: Lock-Free Queue Patterns", "tutorial_lockfree.html", [
      [ "Introduction", "tutorial_lockfree.html#tutorial_lf_intro", null ],
      [ "MPMC vs SPSC: Choosing the Right Queue", "tutorial_lockfree.html#tutorial_lf_mpmc_spsc", null ],
      [ "Hazard Pointer Usage", "tutorial_lockfree.html#tutorial_lf_hazard", null ],
      [ "Performance Characteristics", "tutorial_lockfree.html#tutorial_lf_perf", null ],
      [ "Next Steps", "tutorial_lockfree.html#tutorial_lf_next", null ]
    ] ],
    [ "Frequently Asked Questions", "faq.html", [
      [ "How many threads should I use?", "faq.html#faq_threads", null ],
      [ "How do I handle task cancellation?", "faq.html#faq_cancel", null ],
      [ "Thread pool vs. std::async — which is better?", "faq.html#faq_async", null ],
      [ "How do I integrate with monitoring_system?", "faq.html#faq_monitoring", null ],
      [ "How do I avoid deadlocks when using the pool?", "faq.html#faq_deadlock", null ],
      [ "How do I tune the pool for performance?", "faq.html#faq_perf", null ],
      [ "Are there platform differences I should know about?", "faq.html#faq_platform", null ],
      [ "How does memory management work for jobs and queues?", "faq.html#faq_memory", null ],
      [ "How are errors propagated from jobs?", "faq.html#faq_errors", null ],
      [ "How do I test code that uses the thread pool?", "faq.html#faq_testing", null ]
    ] ],
    [ "Troubleshooting Guide", "troubleshooting.html", [
      [ "Deadlock detection", "troubleshooting.html#ts_deadlock", null ],
      [ "Memory leak with futures", "troubleshooting.html#ts_leak", null ],
      [ "Platform-specific threading issues", "troubleshooting.html#ts_platform", null ],
      [ "Performance problems", "troubleshooting.html#ts_perf", null ],
      [ "Hang on shutdown", "troubleshooting.html#ts_hang", null ],
      [ "More help", "troubleshooting.html#ts_more", null ]
    ] ],
    [ "Deprecated List", "deprecated.html", null ],
    [ "Topics", "topics.html", "topics" ],
    [ "Modules", "modules.html", [
      [ "Modules List", "modules.html", "modules_dup" ],
      [ "Module Members", "modulemembers.html", [
        [ "All", "modulemembers.html", null ],
        [ "Variables", "modulemembers_vars.html", null ]
      ] ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", "namespacemembers_dup" ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", "functions_vars" ],
        [ "Typedefs", "functions_type.html", null ],
        [ "Enumerations", "functions_enum.html", null ],
        [ "Related Symbols", "functions_rela.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ],
        [ "Variables", "globals_vars.html", null ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ],
    [ "Examples", "examples.html", "examples" ]
  ] ]
];

var NAVTREEINDEX =
[
"_2home_2runner_2work_2thread_system_2thread_system_2include_2kcenon_2thread_2core_2atomic_shared_ptr_8h-example.html",
"classkcenon_1_1thread_1_1adaptive__job__queue_1_1accuracy__guard.html#a2bf75502179f79711dd237bf593ed1d2",
"classkcenon_1_1thread_1_1atomic__with__wait.html#a932531a4964e87c0bda75f7ca1b16e6a",
"classkcenon_1_1thread_1_1cancellable__future.html#a9dd89c0f0e0f4a1c4f5697e96efb9de6",
"classkcenon_1_1thread_1_1crash__handler.html#a37b2b78abd12a04416e3ae781646e81c",
"classkcenon_1_1thread_1_1dag__scheduler.html#aaa7226261f67723702361a2ff65bb758",
"classkcenon_1_1thread_1_1detail_1_1thread__impl.html#a8d741152bef3a44bd150e1848688782f",
"classkcenon_1_1thread_1_1job.html#a328bc75ec2c9053c569be348e484189a",
"classkcenon_1_1thread_1_1jobs_1_1job__interface.html#a7eb3c501cea566d83563de403de53790",
"classkcenon_1_1thread_1_1metrics_1_1MetricsBase.html#a4861a7e2d18e350b7e9d4026c329547e",
"classkcenon_1_1thread_1_1policies_1_1adaptive__sync__policy.html#a6f420f2d5a51c21fa6dc1248af2aed68",
"classkcenon_1_1thread_1_1policy__queue.html#a26f1dd8aff49f98e014002955bfc414d",
"classkcenon_1_1thread_1_1queue__factory.html#a4af9248b3c7cd00a3bd554c5cb895109",
"classkcenon_1_1thread_1_1sync_1_1condition__variable__wrapper.html#a15ff3465735608bdd468dc69c7280eee",
"classkcenon_1_1thread_1_1thread__pool.html#ac581647dd03785c943c83e6032677327",
"classkcenon_1_1thread_1_1typed__thread__pool__builder.html#a91d9a1422780e17c213652ce3ec44481",
"classutility__module_1_1convert__string.html#a8d073801a85c823fefc5f2c00e40a2f5",
"dag__job_8h.html#a2eee6c9126f8bb6b06885970b3526cd9",
"group__diagnostics.html#gga2b3d3a928fe423df83fe1452e3f6a3e3a26934eb377001f66e37289a5c93fe284",
"minimal_thread_pool_8cpp-example.html",
"namespacekcenon_1_1thread_1_1concepts.html#a97f1db85190fa0007466df76e53ff725",
"pool__traits_8h.html#aaa62cf7cbecc6a212818bb5f4c542428",
"structkcenon_1_1thread_1_1backpressure__stats.html#a138f2fc1ec6733afbba70342afc469c6",
"structkcenon_1_1thread_1_1detail_1_1function__traits_3_01R_07C_1_1_5_08_07Args_8_8_8_08_4.html#a8e36e062d6be93cb729525a5d2a23b61",
"structkcenon_1_1thread_1_1diagnostics_1_1job__info.html#a75b875ae90e5cd073fd6beee5bcb8dda",
"structkcenon_1_1thread_1_1metrics_1_1WorkerMetrics.html#a0f59c05473a7106e8925ac71e840ef12",
"structkcenon_1_1thread_1_1thread__system__config.html",
"thread__base_8cpp_source.html",
"worker__policy_8h_source.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';