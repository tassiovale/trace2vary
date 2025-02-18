import config
from itertools import combinations
from commonality_and_variability.diff_analysis import calculate_file_differences, get_valid_files, read_files_content

"""trace2vary: An Algorithm to Recover Feature-Code Traceability and Variability

Author: Tassio Vale
Website: www.tassiovale.com
Contact: tassio.vale@ufrb.edu.br

This file has the supporting methods for the C&V analysis process.
"""


def get_all_features(traces_dictionary):
    """
    It builds a set with features available in all analyzed products.
    :param traces_dictionary: resulting traces
    :return: list with all features
    """
    features_set = set()
    for product in traces_dictionary.keys():
        product_features = set(traces_dictionary[product].keys())
        features_set = features_set.union(product_features)
    return list(features_set)


def identify_products_per_feature(features_list, traces_dictionary, trace2vary_results_dictionary):
    """
    It classifies the mandatory and optional features,
    indicating in which products the optional features are available.
    :param features_list: list with all features
    :param traces_dictionary: resulting traces
    :param trace2vary_results_dictionary: data structure to store the final trace2vary output
    """
    for feature in features_list:
        feature_result = dict()
        feature_result[config.result_products] = []
        products_amount = len(traces_dictionary.keys())
        for (product, traces) in traces_dictionary.items():
            if feature in traces.keys():
                feature_result[config.result_products].append(product)
        if len(feature_result[config.result_products]) == products_amount:
            feature_result[config.result_type] = config.mandatory_str
        else:
            feature_result[config.result_type] = config.optional_str
        trace2vary_results_dictionary[feature] = feature_result


def set_file_variability_type_per_feature(traces_dictionary, trace2vary_results_dictionary):
    """
    This method creates the list of common, shared and specific files for all features.
    :param traces_dictionary: resulting traces
    :param trace2vary_results_dictionary: data structure to store the final trace2vary output
    """

    for (feature, feature_result) in trace2vary_results_dictionary.items():
        files_sets = get_all_products_file_lists(traces_dictionary, feature, feature_result)

        # All traced files for a specific feature
        feature_result[config.result_all_files] = list(set.union(*files_sets))
        # All traced files for a specific feature available in all products
        feature_result[config.result_common_files] = list(set.intersection(*files_sets))
        # All traced files for a specific feature available in a single product
        feature_result[config.result_specific_files] = []
        # All traced files for a specific feature available in more than one product (and not in all products)
        feature_result[config.result_shared_files] = []

        for product in feature_result[config.result_products]:
            files_set = set(
                [
                    file.rstrip('/') for file in traces_dictionary[product][feature]
                ]
            )
            for file in files_set:
                if file not in feature_result[config.result_common_files]:
                    if file in feature_result[config.result_specific_files]:
                        feature_result[config.result_specific_files].remove(file)
                        feature_result[config.result_shared_files].append(file)
                    else:
                        feature_result[config.result_specific_files].append(file)


def apply_diff(projects_base_path, trace2vary_results_dictionary):
    """
    It calculate the diff ratio and Levenshtein distance for the file present in more than one product.
    This operation is performed for common and shared files.
    :param projects_base_path: base path related to the projects location
    :param trace2vary_results_dictionary: data structure to store the final trace2vary output
    """

    files_content_dictionary = dict()
    for (feature, feature_result) in trace2vary_results_dictionary.items():

        # Procedure for common files
        feature_result[config.result_common_files_diff_ratios] = dict()
        feature_result[config.result_common_files_levenshtein] = dict()
        for file in feature_result[config.result_common_files]:
            products_files_names = [
                projects_base_path + product + '/' + file for product in feature_result[config.result_products]
            ]
            read_files_content(products_files_names, files_content_dictionary)
            pairs_of_files = list(combinations(products_files_names, 2))
            calculate_file_differences(
                file, pairs_of_files, files_content_dictionary,
                feature_result[config.result_common_files_diff_ratios],
                feature_result[config.result_common_files_levenshtein]
            )

        # Procedure for shared files
        feature_result[config.result_shared_files_diff_ratios] = dict()
        feature_result[config.result_shared_files_levenshtein] = dict()
        for file in feature_result[config.result_shared_files]:
            products_files_names = get_valid_files(projects_base_path, feature_result[config.result_products], file)
            read_files_content(products_files_names, files_content_dictionary)
            pairs_of_files = list(combinations(products_files_names, 2))
            calculate_file_differences(
                file, pairs_of_files, files_content_dictionary,
                feature_result[config.result_shared_files_diff_ratios],
                feature_result[config.result_shared_files_levenshtein]
            )


def get_all_products_file_lists(products_traces_dictionary, feature, feature_result):
    """
    It builds a set with paths for a file present in more than one product.
    :param products_traces_dictionary: resulting traces
    :param feature: the feature took as basis of the analysis
    :param feature_result: data structure with results for the analyzed feature
    :return:
    """
    files_lists = []
    for product in feature_result[config.result_products]:
        files_set = set(
            [
                file.rstrip('/') for file in products_traces_dictionary[product][feature]
            ]
        )
        files_lists.append(files_set)
    return files_lists

