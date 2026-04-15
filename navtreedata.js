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
"classkcenon_1_1thread_1_1configuration__manager.html#ae01aa3aa12af34ac8e594b94e0ee0db8",
"classkcenon_1_1thread_1_1dag__scheduler.html#a6aed14ff0a1553b2cce7e5a0233fc0fe",
"classkcenon_1_1thread_1_1detail_1_1lockfree__job__queue_1_1node__pool.html#aff4c6a3496ac4994af95f53fe1fb0c18",
"classkcenon_1_1thread_1_1hazard__pointer.html",
"classkcenon_1_1thread_1_1job__queue.html#afd4060a0707c949424eb79df3b1ac738",
"classkcenon_1_1thread_1_1metrics_1_1LatencyHistogram.html#a2212916b508132fb044f107de2112705",
"classkcenon_1_1thread_1_1numa__work__stealer.html#a8a841d7d56b2ed28c55e6c17db286710",
"classkcenon_1_1thread_1_1policies_1_1overflow__drop__oldest__policy.html#ac525c0fd9f758648463e5236cd09759e",
"classkcenon_1_1thread_1_1pool__queue__adapter__interface.html#a9de2ee97ab6da2ca1d9c2783190d25f8",
"classkcenon_1_1thread_1_1safe__hazard__guard.html#accaaaafac51661532665703b6e9495be",
"classkcenon_1_1thread_1_1sync_1_1scoped__lock__guard.html#a1a86d8c910aa0c0ade930a9398e1aba5",
"classkcenon_1_1thread_1_1thread__pool.html#aade1d53a410c2fca92e8355fc8f18f05",
"classkcenon_1_1thread_1_1typed__thread__pool__builder.html#a5c1e4f1bb8d91d9973e928db2c47ab56",
"classutility__module_1_1convert__string.html#a406479c216da48dd7d5884c78f5f2097",
"classutility__module_1_1span.html#ac973b445c6c5d76183585eb1126cf03f",
"dag__job__builder_8cpp.html",
"group__diagnostics.html#gga5e12f24370c08f0fe96c982afba8d38ba8377f5fd94f84ea08829f36e1ded3b74",
"metrics__base_8h.html",
"namespacekcenon_1_1thread.html#a87ddb0b6e2c53f951d7bab5a0eab923aa58dcf5004c51af9570779f6fb22a8ad1",
"namespacemembers_r.html",
"structkcenon_1_1thread_1_1autoscaling__policy.html",
"structkcenon_1_1thread_1_1dag__job__info.html#abfe20d3b0ad3afb08eefad06012ac919",
"structkcenon_1_1thread_1_1detail_1_1validate__thread__count.html#a05c15aee2b94e7574044b54b0a915dbe",
"structkcenon_1_1thread_1_1job__info.html#abb377670649992994c7afd11a02dc224",
"structkcenon_1_1thread_1_1scaling__decision.html#a258c00b4000cb03025c2796e6a8b2c8a",
"structkcenon_1_1thread_1_1worker__policy.html#af63800e630f2571a5348ae7e4e34bf32",
"typed__job__queue__sample_8cpp.html#a321ef82abad81c7fedbfe6c419391a01"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';