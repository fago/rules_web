<?php
// $Id$

/**
 * @file Rules Web Hooks - Rules integration.
 */


/**
 * Implements hook_rules_action_info().
 */
function rules_web_hook_rules_action_info() {
  if (rules_web_hook_list()) {
    return array(
      'web_hook_invoke' => array(
        'label' => t('Invoke a web hook'),
        'parameter' => array(
          'hook' => array(
            'type' => 'text',
            'label' => t('Web hook'),
            'options list' => 'rules_web_hook_list',
            'restriction' => 'input',
           ),
        ),
        'group' => t('Rules'),
        'base' => 'rules_web_hook_action_invoke',
      ),
    );
  }
}

/**
 * Returns an option list of web hooks.
 */
function rules_web_hook_list() {
  $hooks = array();
  foreach (entity_load('rules_web_hook', FALSE) as $name => $hook) {
    $hooks[$name] = $hook->label;
  }
  return $hooks;
}

/**
 * Custom process callback for the invoke web hook action.
 */
function rules_web_hook_action_invoke_process(RulesAbstractPlugin $element) {
  $hook = rules_web_hook_load($element->settings['hook']);
  // Add the variables of the hook as parameter.
  foreach ($hook->variables as $name => $param_info) {
    // Prefix parameter names to avoid name clashes with existing parameters.
    $info['parameter']['param_' . $name] = $param_info;
  }
  $element->alterInfo($info);
}

/**
 * Action callback: Invoke a web hook.
 */
function rules_web_hook_action_invoke($hook_name) {
  $hook = rules_web_hook_load($hook_name);
  $args = func_get_args();
  // The state is always the third last parameter that is passed.
  $state = $args[count($args)-3];
  $vars = array();
  foreach ($hook->variables as $name => $info) {
    // Use the args in the state so we can reuse existing rules data wrapper.
    $vars[$name] = $state['execution arguments']['param_' . $name];
  }
  rules_web_hook_invoke($hook, $vars, $state);
}
