<?php
/**
 * @file
 * Renders a entity that references this view via a viewfield.
 */

class viewfield_views_area_handler_area_entity extends views_handler_area {
  public function option_definition() {
    $options = parent::option_definition();
    $options['entity_type'] = array('default' => 'node');
    $options['entity_field'] = array('default' => '');
    $options['entity_bundle'] = array('default' => '');
    $options['view_mode'] = array('default' => 'full');
    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $fields = array();
    $entity_types = array();
    $instances = field_info_instances();
    foreach ($instances as $entity_type => $entity) {
      foreach ($entity as $bundle_name => $bundle){
        if (is_array($bundle) && count($bundle) > 0) {
          foreach ($bundle as $field_name => $field_info) {
            $field = field_info_field($field_name);
            if ($field['type'] == 'viewfield') {
              $fields[$entity_type][$bundle_name][$field['field_name']] = $field['field_name'];
              $entity_types[$entity_type][$bundle_name] = $bundle_name;
            }
          }
        }
      }
    }

    $entity_type_options = array();
    foreach (entity_get_info() as $entity_type => $entity_info) {
      if (isset($entity_types[$entity_type])) {
        $entity_type_options[$entity_type] = $entity_info['label'];
      }
    }

    $entity_type = isset($form_state['input']['options']['entity_type']) ? $form_state['input']['options']['entity_type'] : $this->options['entity_type'];

    $form['entity_type'] = array(
      '#type' => 'select',
      '#title' => t('Entity type'),
      '#options' => $entity_type_options,
      '#description' => t('Choose the entity type you want to display in the area.'),
      '#default_value' => $entity_type,
      '#ajax' => array(
        'path' => views_ui_build_form_url($form_state),
      ),
      '#submit' => array('views_ui_config_item_form_submit_temporary'),
      '#executes_submit_callback' => TRUE,
    );

    if ($entity_type) {
      // Value for bundle we get from options.
      $entity_bundle = $this->options['entity_bundle'];
      // Lets check if $entity_type has $entity_bundle.
      // If not, let's provide $entity_bundle with a suitable value.

      if (!isset($entity_types[$entity_type][$entity_bundle])){
        $entity_bundle = reset($entity_types[$entity_type]);
      }

      $form['entity_bundle'] = array(
        '#type' => 'select',
        '#title' => t('Bundle'),
        '#options' => $entity_types[$entity_type],
        '#description' => t('Choose the entity type you want to display in the area.'),
        '#default_value' => $entity_bundle,
        '#ajax' => array(
          'path' => views_ui_build_form_url($form_state),
        ),
        '#submit' => array('views_ui_config_item_form_submit_temporary'),
        '#executes_submit_callback' => TRUE,
      );

      if ($entity_bundle) {
        $entity_field = $this->options['entity_field'];
        if (isset($fields[$entity_type][$entity_bundle]))
        $form['entity_field'] = array(
          '#type' => 'select',
          '#title' => t('Field'),
          '#options' => $fields[$entity_type][$entity_bundle],
          '#description' => t('Choose the entity type you want to display in the area.'),
          '#ajax' => array(
            'path' => views_ui_build_form_url($form_state),
          ),
          '#submit' => array('views_ui_config_item_form_submit_temporary'),
          '#executes_submit_callback' => TRUE,
        );
        if ($entity_type) {
          $entity_info = entity_get_info($entity_type);
          $options = array();
          if (!empty($entity_info['view modes'])) {
            foreach ($entity_info['view modes'] as $mode => $settings) {
              $options[$mode] = $settings['label'];
            }
          }

          if (count($options) > 1) {
            $form['view_mode'] = array(
              '#type' => 'select',
              '#options' => $options,
              '#title' => t('View mode'),
              '#default_value' => $this->options['view_mode'],
            );
          }
          else {
            $form['view_mode_info'] = array(
              '#type' => 'item',
              '#title' => t('View mode'),
              '#description' => t('Only one view mode is available for this entity type.'),
              '#markup' => $options ? current($options) : t('Default'),
            );
            $form['view_mode'] = array(
              '#type' => 'value',
              '#value' => $options ? key($options) : 'default',
            );
          }
        }
      }
    }

    return $form;
  }

  public function admin_summary() {
    $label = parent::admin_summary();
    if (!empty($this->options['entity_id'])) {
      return t('@label @entity_type:@entity_id', array(
        '@label' => $label,
        '@entity_type' => $this->options['entity_type'],
        '@entity_id' => $this->options['entity_id'],
      ));
    }
  }

  public function render($empty = FALSE) {
    if (!$empty || !empty($this->options['empty'])) {
      $entity_type = $this->options['entity_type'];
      $view_mode = $this->options['view_mode'];
      $view = $this->view;
      $display_id = $this->view->current_display;
      $view_name = $view->name;
      $entity_field = $this->options['entity_field'];
      $entity_bundle = $this->options['entity_bundle'];
      // Getting nodes with the same title as current view's machine name.
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', $entity_type)
        ->entityCondition('bundle', $entity_bundle)
        ->fieldCondition($entity_field, 'vname', $view_name . '|' . $display_id);

      $result = $query->execute();
      // if there are results, we'll add the 'views_header' view mode as header of the current view.
      if (isset($result[$entity_type])) {
        $header_nids = array_keys($result[$entity_type]);
        $header_items = entity_load($entity_type, $header_nids);
        // Getting the first one.
        $entity = array_pop($header_items);
        // Getting it as "views_header" view mode.
        $build = entity_view($entity_type, array($entity), $view_mode);

        return drupal_render($build);
      }
      else {
        if ($this->montjuic_list_get_entity_creation_permission($entity_type, $entity_bundle)) {
          $info = entity_get_info($entity_type);
          $uri = $this->montjuic_list_get_entity_creation_uri($entity_type, $entity_bundle);
          if ($uri) {
            $viewfield_argument = $view_name . '|' . $display_id;
            $query_arguments = array(
              'viewsfield' => $viewfield_argument,
              'viewsfield_field' => $entity_field,
            );

            $query_arguments += drupal_get_destination();
            return t("There's no entity pointing to this view. <a href='@url'>You can create one now if you want.</a>", array('@url' => url($uri, array('query' => $query_arguments))));
          }
        }
      }
    }
    return '';
  }

  /**
   * Decides access to content creation link in case of an empty header.
   */
  public function montjuic_list_get_entity_creation_permission($entity_type, $entity_bundle) {
    // By default, deny access.
    $access = FALSE;
    switch ($entity_type) {
      case 'node':
        $access = user_access('create ' . $entity_bundle . ' content');
        break;
      case 'taxonomy_term':
        $access = user_access('administer taxonomy');
      default:
        $access = user_access('create ' . $entity_type . ' ' . $entity_bundle);
        break;
    }

    // Give other modules the opportunity to alter the permissions.
    drupal_alter('viewfield_access', $uri, $context);

    return $access;
  }

  /**
   * Decides uri to content creation link in case of an empty header.
   */
  public function montjuic_list_get_entity_creation_uri($entity_type, $entity_bundle) {
    // Argument to automatically populate field.
    switch ($entity_type) {
      case 'node':
        $entity_bundle = str_replace('_', '-', $entity_bundle);
        $uri = $entity_type . '/add/' . $entity_bundle;
        break;

      case 'taxonomy_term':
        $uri = 'admin/structure/taxonomy/' . $entity_bundle . '/add';
        break;

      default:
        $uri = $entity_type . '/add/' . $entity_bundle;
        break;
    }

    $context = array(
      'entity_type' => $entity_type,
      'entity_bundle' => $entity_bundle
    );

    // Give other modules the opportunity to alter the uri.
    drupal_alter('viewfield_uri', $uri, $context);

    return $uri;
  }

}
