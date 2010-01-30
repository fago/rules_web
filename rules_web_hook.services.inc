<?php
// $Id$

/**
 * @file Rules Web Hooks - Provides services.
 */


/**
 * Implements hook_services_resources().
 */
function rules_web_hook_services_resources() {
  $crud_object = new EntityResourceServicesCRUD('rules_web_hook');
  $resources['rules_web_hook']['index'] = array(
    'file' => array('type' => 'inc', 'module' => 'rules_web_hook', 'name' => 'rules_web_hook.services'),
    'callback' => 'rules_web_hook_list_hooks',
    'args' => array(),
    'models' => array(),
    'access callback' => array($crud_object, 'access'),
    'access arguments' => array('view'),
  );
  $resources['entity_metadata']['index'] = array(
    'file' => array('type' => 'inc', 'module' => 'rules_web_hook', 'name' => 'rules_web_hook.services'),
    'callback' => 'rules_web_hook_list_entity_metadata',
    'args' => array(),
    'models' => array(),
    'access callback' => array($crud_object, 'access'),
    'access arguments' => array('view'),
  );
  return $resources;
}

/**
 * Service resource callback: Index all web hooks.
 */
function rules_web_hook_list_hooks() {
  return entity_load('rules_web_hook', FALSE, array('active' => TRUE));
}

/**
 * Service resource callback: List metadata about entites, but only if the
 * current user has access to use it - so the user isn't aware of unaccesible
 * entities / properties.
 */
function rules_web_hook_list_entity_metadata($account = NULL) {
  $info = entity_get_info();
  $return = array();
  foreach ($info as $type => $entity_info) {
    if (entity_metadata_entity_access('view', $type, NULL, $account)) {
      $return[$type] = array_intersect_key($entity_info, array_flip(array('label', 'object keys')));
      $wrapper = entity_metadata_wrapper($type);
      $metadata = $wrapper->dataInfo();
      foreach ($wrapper as $name => $info) {
        if ($wrapper->$name->access('view', $account)) {
          $return[$type]['properties'][$name] = $info;
        }
      }
      // Add in bundle properties
      foreach ($metadata['bundles'] as $bundle => $bundle_info) {
        $return[$type]['bundles'][$bundle] = array_intersect_key($bundle_info, array_flip(array('label')));

        if (!empty($bundle_info['properties'])) {
          $wrapper = entity_metadata_wrapper($type, NULL, array('bundle' => $bundle));
          foreach ($bundle_info['properties'] as $name => $info) {
            if ($wrapper->$name->access('view', $account)) {
              $return[$type]['bundles'][$bundle]['properties'][$name] = $info;
            }
          }
        }
      }
    }
  }
  return $return;
}
