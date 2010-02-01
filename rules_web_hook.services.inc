<?php
// $Id$

/**
 * @file Rules Web Hooks - Provides services.
 */


/**
 * Implements hook_services_resources().
 */
function rules_web_hook_services_resources() {
  $crud_object = EntityResourceServicesCRUD::get('rules_web_hook');
  $crud_object->callbacks['subscribe'] = 'rules_web_hook_subscribe';
  $crud_object->callbacks['unsubscribe'] = 'rules_web_hook_unsubscribe';

  $resources['rules_web_hook']['index'] = array(
    'file' => array('type' => 'inc', 'module' => 'rules_web_hook', 'name' => 'rules_web_hook.services'),
    'callback' => 'rules_web_hook_list_hooks',
    'args' => array(),
    'models' => array(),
    'access callback' => array($crud_object, 'access'),
    'access arguments' => array('view'),
  );

  $resources['rules_web_hook']['targeted actions']['subscribe'] = array(
    'file' => array('type' => 'inc', 'module' => 'rules_web_hook', 'name' => 'rules_web_hook.services'),
    'callback' => array($crud_object, 'subscribe'),
    'args' => array(
      array(
        'name' => 'id',
        'optional' => FALSE,
        'source' => array('path' => 0),
      ),
      array(
        'name' => 'url',
        'source' => 'data',
        'description' => 'The URL where to post notifications to.',
        'type' => 'string',
      ),
    ),
    'models' => array(),
    'access callback' => array($crud_object, 'access'),
    'access arguments' => array('view'),
  );

  $resources['rules_web_hook']['targeted actions']['unsubscribe'] = array(
    'callback' => array($crud_object, 'unsubscribe'),
  ) + $resources['rules_web_hook']['targeted actions']['subscribe'];

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
          $return[$type]['properties'][$name] = array_intersect_key($info, array_flip(array('label', 'type', 'description', 'sanitize')));
        }
      }
      // Add in bundle properties
      foreach ($metadata['bundles'] as $bundle => $bundle_info) {
        $return[$type]['bundles'][$bundle] = array_intersect_key($bundle_info, array_flip(array('label')));

        if (!empty($bundle_info['properties'])) {
          $wrapper = entity_metadata_wrapper($type, NULL, array('bundle' => $bundle));
          foreach ($bundle_info['properties'] as $name => $info) {
            if ($wrapper->$name->access('view', $account)) {
              $return[$type]['bundles'][$bundle]['properties'][$name] = array_intersect_key($info, array_flip(array('label', 'type', 'description', 'sanitize')));
            }
          }
        }
      }
    }
  }
  return $return;
}


/**
 * Service resource action callback invoked via the entity resource crud object.
 */
function rules_web_hook_subscribe($crud_object, $args) {
  if ($hook = $crud_object->load($args[0])) {
    $url = $args[1];

    try {
      $client = new RestClient(NULL, new RestClientBaseFormatter(RestClientBaseFormatter::FORMAT_JSON));
      $client->post($url, array(
        'handshake' => TRUE,
      ));

      db_merge('rules_web_hook_subscriber')
        ->key(array('id' => $hook->id, 'url' => $url))
        ->fields(array('uid' => $GLOBALS['user']->uid))
        ->execute();

      return TRUE;
    }
    catch (Exception $e) {
      // Troubles connecting to $url.
      services_error('Connecting to provided URL fails.', 406);
    }
  }
  services_error('Hook not found.', 404);
}

/**
 * Service resource action callback invoked via the entity resource crud object.
 */
function rules_web_hook_unsubscribe($crud_object, $args) {
  $hook = $crud_object->load($args[0]);
  $url = $args[1];

  db_delete('rules_web_hook_subscriber')
    ->condition('id', $hook->id)
    ->condition('url', $url)
    ->execute();

  return TRUE;
}
